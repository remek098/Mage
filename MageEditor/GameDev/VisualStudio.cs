using MageEditor.Utilities;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices.ComTypes;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;
using System.IO;

namespace MageEditor.GameDev
{
    static class VisualStudio
    {
        // https://learn.microsoft.com/en-us/dotnet/api/envdte80?view=visualstudiosdk-2022
        // https://learn.microsoft.com/en-us/dotnet/api/envdte80.dte2?view=visualstudiosdk-2022
        private static EnvDTE80.DTE2? _vsInscance = null;

        // VisualStudio.DTE.17.0 for VS2022
        private static readonly string _progID = "VisualStudio.DTE";


        // https://learn.microsoft.com/en-us/windows/win32/api/objbase/nf-objbase-getrunningobjecttable
        [DllImport("ole32.dll")]
        private static extern int GetRunningObjectTable(uint reserved, out IRunningObjectTable pprot);


        // https://learn.microsoft.com/en-us/windows/win32/api/objbase/nf-objbase-createbindctx
        [DllImport("ole32.dll")]
        private static extern int CreateBindCtx(uint reserved, out IBindCtx ppbc);

        public static void OpenVisualStudio(string solutionPath)
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
                            var solutionName = dte.Solution.FullName;
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
                            _vsInscance = Activator.CreateInstance(visualStudioType) as EnvDTE80.DTE2;

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


        public static void CloseVisualStudio()
        {
            // save just in case and close visual studio instance
            if(_vsInscance?.Solution.IsOpen == true)
            {
                // Ctrl+Q -> Keybindings -> File.SaveAll -> Ctrl+Shift+S (Global)
                // NOTE: commands like Build.BuildSolution exist as well.
                _vsInscance.ExecuteCommand("File.SaveAll");
                _vsInscance.Solution.Close(true);
            }
            _vsInscance?.Quit();
        }

        /// <summary>
        /// Adds files to solution and returns true if it succeeded.
        /// </summary>
        /// <param name="solution">Solution's path that you with the files to be added to.</param>
        /// <param name="projectName"></param>
        /// <param name="files">string[] containing paths to .h and .cpp files</param>
        /// <returns></returns>
        public static bool AddFilesToSolution(string solution, string projectName, string[] files)
        {
            Debug.Assert(files?.Length > 0);
            OpenVisualStudio(solution);
            try
            {
                if(_vsInscance != null)
                {
                    // doing SaveAll in case visual studio would crash during adding files.
                    if (!_vsInscance.Solution.IsOpen) _vsInscance.Solution.Open(solution);
                    else _vsInscance.ExecuteCommand("File.SaveAll");

                    foreach(EnvDTE.Project project in _vsInscance.Solution.Projects)
                    {
                        // if we found a project that at the very least contains the name of our project
                        if(project.UniqueName.Contains(projectName))
                        {
                            // add files 1 by 1 to  that project.
                            foreach(var file in files)
                            {
                                project.ProjectItems.AddFromFile(file);
                            }
                        }
                    }



                    // open added cpp file(s)
                    var cpp_files = files.FirstOrDefault(x => Path.GetExtension(x) == ".cpp");
                    if(string.IsNullOrEmpty(cpp_files))
                    {
                        // NOTE: for MageEditor > Dependencies > COM > Interop.EnvDTE you gotta set in properties "Embed Interop Types" to "No"
                        // otherwise if it's left as Yes, you gotta use "{7651A703-06E5-11D1-8EBD-00A0C90F26EA}" // vsViewKindTextView
                        _vsInscance.ItemOperations.OpenFile(cpp_files, EnvDTE.Constants.vsViewKindTextView).Visible = true;
                    }
                    _vsInscance.MainWindow.Activate();
                    _vsInscance.MainWindow.Visible = true;

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
    }
}
