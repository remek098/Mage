using EnvDTE;
using MageEditor.GameProject;
using MageEditor.Utilities;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.ComTypes;

namespace MageEditor.GameDev
{
    using GameProject;
    using System.Collections;
    using System.Collections.Concurrent;
    using System.Threading;
    enum BuildConfiguration
    {
        Debug,
        DebugEditor,
        Release,
        ReleaseEditor,
    }


    static class VisualStudio
    {
        private static readonly ManualResetEventSlim _resetEvent = new ManualResetEventSlim(false);
        private static readonly object _lock = new object();
        // https://learn.microsoft.com/en-us/dotnet/api/envdte80?view=visualstudiosdk-2022
        // https://learn.microsoft.com/en-us/dotnet/api/envdte80.dte2?view=visualstudiosdk-2022
        private static EnvDTE80.DTE2? _vsInscance = null;

        // VisualStudio.DTE.17.0 for VS2022
        private static readonly string _progID = "VisualStudio.DTE";
        
        private static EnvDTE.BuildEvents? _buildEvents;
        private static bool _eventsSubscribed = false;

        public static bool BuildSucceded { get; private set; } = true;
        public static bool BuildDone { get; private set; } = true;

        private static readonly string[] _buildConfigurationNames = new string[] { "Debug", "DebugEditor", "Release", "ReleaseEditor" };
        /*
        returns BuildConfiguration enum type that we have chosen in Editor with WorldViewEditor's ComboBox x:Name="runConfig"
        if used with StandaloneBuildConfig property for standalone app
        and DllBuildConfig property for Editor's game dll
        */
        public static string GetConfigurationName(BuildConfiguration configuration) => _buildConfigurationNames[(int)configuration];



        // https://learn.microsoft.com/en-us/windows/win32/api/objbase/nf-objbase-getrunningobjecttable
        [DllImport("ole32.dll")]
        private static extern int GetRunningObjectTable(uint reserved, out IRunningObjectTable pprot);


        // https://learn.microsoft.com/en-us/windows/win32/api/objbase/nf-objbase-createbindctx
        [DllImport("ole32.dll")]
        private static extern int CreateBindCtx(uint reserved, out IBindCtx ppbc);


        private static void SubscribeBuildEvents()
        {
            var sw = Stopwatch.StartNew();
            if (_vsInscance == null || _eventsSubscribed) return;
            var events = _vsInscance.Events.BuildEvents;
            events.OnBuildProjConfigBegin += OnBuildProjectBegin;
            events.OnBuildProjConfigDone += OnBuildProjectDone;
            _eventsSubscribed = true;
            _buildEvents = events;
        }

        private static void UnsubscribeBuildEvents()
        {
            if(_buildEvents == null) return;
            // if (_vsInscance == null || !_eventsSubscribed) return;
            // _buildEvents = _vsInscance.Events.BuildEvents;
            _buildEvents.OnBuildProjConfigBegin -= OnBuildProjectBegin;
            _buildEvents.OnBuildProjConfigDone -= OnBuildProjectDone;

            Marshal.ReleaseComObject(_buildEvents);
            _buildEvents = null;
            _eventsSubscribed = false;
        }


        private static void CallOnSTAThread(Action action)
        {
            Debug.Assert(action != null);
            var thread = new Thread(() =>
            {
                MessageFilter.Register();
                try { action(); }
                catch (Exception ex) { Logger.Log(MessageType.Warning, ex.Message); }
                finally { MessageFilter.Revoke(); }
            });


            thread.SetApartmentState(ApartmentState.STA);
            thread.Start(); // this will imidiately run our thread function.
            thread.Join(); // wait until the action is done.
        }



        private static void OpenVisualStudio_Internal(string solutionPath)
        {
            // COM objects -> gotta release them to lower reference count
            IRunningObjectTable? rot = null;
            IEnumMoniker? monikerTable = null;
            IBindCtx? bindCtx = null;
            try
            {
                if(_vsInscance == null)
                {
                    // find and open Visual Studio if it's already running
                    var hresult = GetRunningObjectTable(0, out rot);
                    // user can lookup MSDN or internet for a returned HRESULT
                    if (hresult < 0 || rot == null) throw new COMException($"GetRunningObjectTable() returned HRESULT: {hresult:X8}");

                    rot.EnumRunning(out monikerTable);
                    monikerTable.Reset();

                    hresult = CreateBindCtx(0, out bindCtx);
                    if (hresult < 0 || bindCtx == null) throw new COMException($"CreateBindingCtx() returned HRESULT: {hresult:X8}");

                    IMoniker[] currentMoniker = new IMoniker[1];
                    while(monikerTable.Next(1, currentMoniker, IntPtr.Zero) == 0)
                    {
                        string name = string.Empty;
                        currentMoniker[0]?.GetDisplayName(bindCtx, null, out name);
                        // if the name contains _progID, we found visual studio instance running
                        if(name.Contains(_progID))
                        {
                            // we need the one that has our game solution opened
                            hresult = rot.GetObject(currentMoniker[0], out object obj);
                            if (hresult < 0 || obj == null) throw new COMException($"Running object table's GetObject returned HRESULT: {hresult:X8}");

                            EnvDTE80.DTE2 dte = (EnvDTE80.DTE2)obj;
                            var solutionName = string.Empty;
                            CallOnSTAThread(() =>
                            {
                                solutionName = dte.Solution.FullName;
                            });

                            if (solutionName == solutionPath)
                            {
                                _vsInscance = dte;
                                break;
                            }
                        }
                    }
                    // start a Visual Studio program
                    if (_vsInscance == null)
                    {
                        Type? visualStudioType = Type.GetTypeFromProgID(_progID, true);
                        if(visualStudioType is not null)
                        {
                            _vsInscance = Activator.CreateInstance(visualStudioType) as EnvDTE80.DTE2;
                        }

                    }
                }
            }
            catch(Exception ex)
            {
                Debug.WriteLine(ex.Message);
                Logger.Log(MessageType.Error, "Failed to open Visual Studio");
            }
            finally
            {
                if(monikerTable != null) Marshal.ReleaseComObject(monikerTable);
                if (rot != null) Marshal.ReleaseComObject(rot);
                if(bindCtx != null) Marshal.ReleaseComObject(bindCtx);
            }
        }

        public static void OpenVisualStudio(string solutionPath)
        {
            lock(_lock) { OpenVisualStudio_Internal(solutionPath); }
        }


        private static void CloseVisualStudio_Internal()
        {
            CallOnSTAThread(() =>
            {
                // save just in case and close visual studio instance
                if (_vsInscance?.Solution.IsOpen == true) {
                    // Ctrl+Q -> Keybindings -> File.SaveAll -> Ctrl+Shift+S (Global)
                    // NOTE: commands like Build.BuildSolution exist as well.
                    _vsInscance.ExecuteCommand("File.SaveAll");
                    _vsInscance.Solution.Close(true);
                }
                _vsInscance?.Quit();
                _vsInscance = null;
            });
        }

        public static void CloseVisualStudio()
        {
            lock (_lock) { CloseVisualStudio_Internal(); }

        }


        /// <summary>
        /// Adds files to solution and returns true if it succeeded.
        /// </summary>
        /// <param name="solution">Solution's path that you with the files to be added to.</param>
        /// <param name="projectName"></param>
        /// <param name="files">string[] containing paths to .h and .cpp files</param>
        /// <returns></returns>
        private static bool AddFilesToSolution_Internal(string solution, string projectName, string[] files)
        {
            Debug.Assert(files?.Length > 0);
            OpenVisualStudio_Internal(solution);
            try
            {
                if(_vsInscance != null)
                {
                    CallOnSTAThread(() =>
                    {
                        // doing SaveAll in case visual studio would crash during adding files.
                        if (!_vsInscance.Solution.IsOpen) _vsInscance.Solution.Open(solution);
                        // else _vsInscance.ExecuteCommand("File.SaveAll");

                        foreach (EnvDTE.Project project in _vsInscance.Solution.Projects) {
                            // if we found a project that at the very least contains the name of our project
                            if (project.UniqueName.Contains(projectName)) {
                                // add files 1 by 1 to  that project.
                                foreach (var file in files) {
                                    project.ProjectItems.AddFromFile(file);
                                }
                            }
                        }
                        _vsInscance.ExecuteCommand("File.SaveAll");


                        // open added cpp file(s)
                        bool opened_any_files = false;
                        var cpp_files = files.FirstOrDefault(x => Path.GetExtension(x) == ".cpp");
                        var h_files = files.FirstOrDefault(x => Path.GetExtension(x) == ".h");
                        if (!string.IsNullOrEmpty(cpp_files)) {
                            // NOTE: for MageEditor > Dependencies > COM > Interop.EnvDTE you gotta set in properties "Embed Interop Types" to "No"
                            // otherwise if it's left as Yes, you gotta use "{7651A703-06E5-11D1-8EBD-00A0C90F26EA}" // vsViewKindTextView
                            _vsInscance.ItemOperations.OpenFile(cpp_files, EnvDTE.Constants.vsViewKindTextView).Visible = true;
                            _vsInscance.ItemOperations.OpenFile(h_files, EnvDTE.Constants.vsViewKindTextView).Visible = true;
                            opened_any_files = true;
                        }
                        if (opened_any_files) {
                            foreach (var file in files) Logger.Log(MessageType.Info, $"Opened file: {file}");
                        }
                        else {
                            foreach (var file in files) Logger.Log(MessageType.Error, $"Could not open file: {file}");
                        }

                        _vsInscance.MainWindow.Activate();
                        _vsInscance.MainWindow.Visible = true;
                    });
                }
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
                Debug.WriteLine("Failed to add files to Visual Studio project");
                return false;
            }

            return true;
        }

        public static bool AddFilesToSolution(string solution, string projectName, string[] files)
        {
            lock(_lock) { return AddFilesToSolution_Internal(solution, projectName, files); }
        }


        private static void OnBuildProjectDone(string project, string projectConfig, string platform, string solutionConfig, bool success)
        {
            //if (_vsInscance != null) {
            //    _vsInscance.Events.BuildEvents.OnBuildProjConfigBegin -= OnBuildProjectBegin;
            //    _vsInscance.Events.BuildEvents.OnBuildProjConfigDone -= OnBuildProjectDone;
            //}
            if (BuildDone) return;

            if (success) Logger.Log(MessageType.Info, $"Building {projectConfig} configuration succeded");
            else Logger.Log(MessageType.Error, $"Building {projectConfig} configuration failed");

            BuildDone = true;
            BuildSucceded = success;
            _resetEvent.Set();
            UnsubscribeBuildEvents();
        }

        private static void OnBuildProjectBegin(string project, string projectConfig, string platform, string solutionConfig)
        {
            if (BuildDone) return;
            Logger.Log(MessageType.Info, $"Building {project}, {projectConfig}, {platform}, {solutionConfig}");
        }

        private static bool IsDebugging_Internal()
        {
            bool result = false;

            CallOnSTAThread(() =>
            {
                // debugger is either debugging current program or running it already if result is true
                result = _vsInscance != null && 
                    (_vsInscance?.Debugger.CurrentProgram != null || _vsInscance?.Debugger.CurrentMode == EnvDTE.dbgDebugMode.dbgRunMode);
            });
            
            
            return result;
        }

        public static bool IsDebugging()
        {
            lock(_lock) { return IsDebugging_Internal(); }
        }

        private static void BuildSolution_Internal(Project project, BuildConfiguration buildConfig, bool showVSWindow = true)
        {
            if(IsDebugging_Internal())
            {
                Logger.Log(MessageType.Error, "Visual Studio is currently running the process.");
                return;
            }

            OpenVisualStudio_Internal(project.Solution);
            BuildDone = BuildSucceded = false;

            CallOnSTAThread(() =>
            {
                // if (_vsInscance != null && !_vsInscance.Solution.IsOpen) _vsInscance.Solution.Open(project.Solution);
                _vsInscance?.Solution.Open(project.Solution);
                _vsInscance?.MainWindow.Visible = showVSWindow;
                SubscribeBuildEvents();
            });

            var configName = GetConfigurationName(buildConfig);
            // try to delete all pdb files when we build a solution;
            // NOTE: game code dll depends on pdb file it's associated with
            // just kinda trying to keep output directory clean (to not have hundreds of pdb files for no reason)
            // NOTE: works only because we rebuild a new dll anyway.
            try
            {
                foreach(var pdbFile in Directory.GetFiles(Path.Combine($"{project.Path}", $@"x64\{configName}"), "*pdb"))
                {
                    File.Delete(pdbFile);
                }
            }
            catch(Exception ex) { Debug.WriteLine(ex.Message); }

            CallOnSTAThread(() =>
            {
                _vsInscance?.Solution.SolutionBuild.SolutionConfigurations.Item(configName).Activate();
                _vsInscance?.ExecuteCommand("Build.BuildSolution"); // issue Visual Studio command
                _resetEvent.Wait();
                _resetEvent.Reset();
            });
        }

        public static void BuildSolution(Project project, BuildConfiguration buildConfig, bool showVSWindow = true)
        {
            lock(_lock) { BuildSolution_Internal(project, buildConfig, showVSWindow); }
        }


        /// <summary>
        /// run the game code solution
        /// </summary>
        /// <param name="project"></param>
        /// <param name="configName"></param>
        /// <param name="debug">Start with debugger if true, otherwise start without debugging.</param>
        private static void Run_Internal(Project project, BuildConfiguration buildConfig, bool debug)
        {
            CallOnSTAThread(() =>
            {
                if (_vsInscance != null && !IsDebugging_Internal() && BuildSucceded) {
                    _vsInscance.ExecuteCommand(debug ? "Debug.Start" : "Debug.StartWithoutDebugging");
                }
            });
        }

        public static void Run(Project project, BuildConfiguration buildConfig, bool debug)
        {
            lock(_lock) { Run_Internal(project, buildConfig, debug); }
        }

        private static void Stop_Internal()
        {
            CallOnSTAThread(() =>
            {
                if (_vsInscance != null && IsDebugging_Internal())
                {
                    _vsInscance.ExecuteCommand("Debug.StopDebugging");
                }
            });
        }

        public static void Stop()
        {
            lock(_lock) { Stop_Internal(); }
        }
    }

    /// <summary>
    /// Class containing the IOleMessageFilter thread error-handling function.
    /// </summary>
    public class MessageFilter : IOleMessageFilter
    {
        private const int SERVERCALL_ISHANDLED = 0;
        private const int PENDINGMSG_WAITDEFPROCESS = 2;
        private const int SERVERCALL_RETRYLATER = 2;

        // implement IOleMessageFilter interface. 
        [DllImport("Ole32.dll")]
        private static extern int CoRegisterMessageFilter(IOleMessageFilter? newFilter, out IOleMessageFilter oldFilter);

        public static void Register()
        {
            IOleMessageFilter newFilter = new MessageFilter();
            int hr = CoRegisterMessageFilter(newFilter, out var oldFilter);
            Debug.Assert(hr >= 0, "Registering COM IMessageFilter failed.");
        }


        public static void Revoke()
        {
            int hr = CoRegisterMessageFilter(null, out var oldFilter);
            Debug.Assert(hr >= 0, "Unregistering COM IMessageFilter failed.");
        }

        int IOleMessageFilter.HandleInComingCall(int dwCallType, System.IntPtr hTaskCaller, int dwTickCount, System.IntPtr lpInterfaceInfo)
        {
            return SERVERCALL_ISHANDLED;
        }


        int IOleMessageFilter.RetryRejectedCall(System.IntPtr hTaskCallee, int dwTickCount, int dwRejectType)
        {
            // Thread call was refused, try again. 
            if (dwRejectType == SERVERCALL_RETRYLATER)
            // flag = SERVERCALL_RETRYLATER. 
            {
                // retry thread call at once, if return value >=0 & <100. 
                Debug.WriteLine("COM server busy. Retrying call to EnvDTE interface.");
                return 500;
            }
            // Too busy. Cancel call.
            return -1;
        }


        int IOleMessageFilter.MessagePending(System.IntPtr hTaskCallee, int dwTickCount, int dwPendingType)
        {
            //return flag PENDINGMSG_WAITDEFPROCESS. 
            return PENDINGMSG_WAITDEFPROCESS;
        }
    }



    [ComImport(), Guid("00000016-0000-0000-C000-000000000046"), 
    InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    interface IOleMessageFilter
    {

        [PreserveSig]
        int HandleInComingCall(int dwCallType, IntPtr hTaskCaller, int dwTickCount, IntPtr lpInterfaceInfo);


        [PreserveSig]
        int RetryRejectedCall(IntPtr hTaskCallee, int dwTickCount, int dwRejectType);


        [PreserveSig]
        int MessagePending(IntPtr hTaskCallee, int dwTickCount, int dwPendingType);
    }
}
