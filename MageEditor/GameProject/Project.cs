using System;
using System.Windows;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;
using System.Diagnostics;
using System.IO;
using MageEditor.Utilities;
using System.Windows.Input;
using MageEditor.Common;
using MageEditor.GameDev;
using MageEditor.DllWrappers;
using MageEditor.Components;

namespace MageEditor.GameProject
{
    enum BuildConfiguration
    {
        Debug,
        DebugEditor,
        Release,
        ReleaseEditor,
    }


    [DataContract(Name = "Game")]
    class Project : ViewModelBase
    {
        public static string Extension { get; } = ".mage";

        [DataMember]
        public string Name { get; private set; } = "New Project";

        [DataMember]
        public string Path { get; private set; }

        public string FullPath => $@"{Path}{Name}{Extension}";
        public string Solution => $@"{Path}{Name}.sln";
        public string ContentPath => $@"{Path}Content\";

        private static readonly string[] _buildConfigurationNames = new string[] { "Debug", "DebugEditor", "Release", "ReleaseEditor" };

        public int _buildConfig;

        [DataMember]
        public int BuildConfig
        {
            get => _buildConfig;
            set
            {
                if(_buildConfig != value)
                {
                    _buildConfig = value;
                    OnPropertyChanged(nameof(BuildConfig));
                }
            }
        }

        // for standalone application
        public BuildConfiguration StandaloneBuildConfig => BuildConfig == 0 ? BuildConfiguration.Debug : BuildConfiguration.Release;
        public BuildConfiguration DllBuildConfig => BuildConfig == 0 ? BuildConfiguration.DebugEditor : BuildConfiguration.ReleaseEditor;

        // NOTE: filled in when we load in game code dll (so in Project.LoadGameCodeDll() method)
        public string[]? _availableScriptNames;
        public string[]? AvailableScriptNames
        {
            get => _availableScriptNames;
            set
            {
                if(_availableScriptNames != value)
                {
                    _availableScriptNames = value;
                    OnPropertyChanged(nameof(AvailableScriptNames));
                }
            }
        }

        [DataMember(Name = "Scenes")]
        private ObservableCollection<Scene> _scenes = new ObservableCollection<Scene>();

        public ReadOnlyObservableCollection<Scene> Scenes 
        { get; private set; }

        private Scene? _activeScene;

        
        public Scene? ActiveScene
        {
            get => _activeScene;
            set
            {
                if (_activeScene != value)
                {
                    _activeScene = value;
                    OnPropertyChanged(nameof(ActiveScene));
                }
            }
        }


        public static Project? Current => Application.Current.MainWindow.DataContext as Project;
        public static UndoRedo UndoRedo { get; } = new UndoRedo();

        public ICommand UndoCommand { get; private set; }
        public ICommand RedoCommand { get; private set; }

        public ICommand AddSceneCommand { get; private set; }
        public ICommand RemoveSceneCommand {  get; private set; }

        public ICommand SaveCommand { get; private set; }

        /// <summary>
        /// start game application in the debugger.
        /// </summary>
        public ICommand DebugStartCommand { get; private set; }
        /// <summary>
        /// start game application without debugging.
        /// </summary>
        public ICommand DebugStartWithoutDebuggingCommand { get; private set; }
        /// <summary>
        /// stop game application if running in debugger.
        /// </summary>
        public ICommand DebugStopCommand { get; private set; }
        public ICommand BuildCommand { get; private set; }

        private void SetCommands()
        {
            AddSceneCommand = new RelayCommand<object>(x =>
            {
                AddScene($"New Scene {_scenes?.Count}");
                var newScene = _scenes?.Last();
                var sceneIndex = _scenes?.Count - 1;


                if (newScene != null && sceneIndex.HasValue)
                {
                    UndoRedo.Add(new UndoRedoAction(
                        () => RemoveScene(newScene),
                        () => _scenes?.Insert(sceneIndex.Value, newScene),
                        $"Add {newScene?.Name}"
                    ));
                }

            });

            RemoveSceneCommand = new RelayCommand<Scene>(x =>
            {
                var sceneIndex = _scenes?.IndexOf(x);
                RemoveScene(x);

                if (sceneIndex.HasValue)
                    UndoRedo.Add(new UndoRedoAction(
                        () => _scenes?.Insert(sceneIndex.Value, x),
                        () => RemoveScene(x),
                        $"Remove {x.Name}"
                    ));

            },
            x => !x.IsActive);

            UndoCommand = new RelayCommand<object>(x => UndoRedo.Undo(), x => UndoRedo.UndoList.Any());
            RedoCommand = new RelayCommand<object>(x => UndoRedo.Redo(), x => UndoRedo.RedoList.Any());
            SaveCommand = new RelayCommand<object>(x => Save(this));
            
            DebugStartCommand = new RelayCommand<object>(async x => await RunGame(true), x => !VisualStudio.IsDebugging() && VisualStudio.BuildDone);
            DebugStartWithoutDebuggingCommand = new RelayCommand<object>(async x => await RunGame(false), x => !VisualStudio.IsDebugging() && VisualStudio.BuildDone);
            DebugStopCommand = new RelayCommand<object>(async x => await StopGame(), x => VisualStudio.IsDebugging());

            // if visual studio is already running this command, we cannot use this command until it's done
            BuildCommand = new RelayCommand<bool>(async x => await BuildGameCodeDll(x), x => !VisualStudio.IsDebugging() && VisualStudio.BuildDone);


            // NOTE: Have to do this, because initially we await in OnDeserialized() -> when launching editor.
            // this will inform UI that we got commands initialized,
            // those will be bound to buttons, etc. in editor
            OnPropertyChanged(nameof(AddSceneCommand));
            OnPropertyChanged(nameof(RemoveSceneCommand));
            OnPropertyChanged(nameof(UndoCommand));
            OnPropertyChanged(nameof(RedoCommand));
            OnPropertyChanged(nameof(SaveCommand));
            OnPropertyChanged(nameof(DebugStartCommand));
            OnPropertyChanged(nameof (DebugStartWithoutDebuggingCommand));
            OnPropertyChanged(nameof(DebugStopCommand));
            OnPropertyChanged(nameof(BuildCommand));
        }

        /*
        returns BuildConfiguration enum type that we have chosen in Editor with WorldViewEditor's ComboBox x:Name="runConfig"
        if used with StandaloneBuildConfig property for standalone app
        and DllBuildConfig property for Editor's game dll
        */
        private static string GetConfigurationName(BuildConfiguration configuration) => _buildConfigurationNames[(int)configuration];

        private void AddScene(string sceneName)
        {
            Debug.Assert(!string.IsNullOrEmpty(sceneName.Trim()));
            _scenes.Add(new Scene(this, sceneName));
        }

        private void RemoveScene(Scene scene)
        {
            Debug.Assert(_scenes.Contains(scene));
            _scenes.Remove(scene);
        }

        public static Project? Load(string file)
        {
            Debug.Assert(File.Exists(file));
            return Serializer.FromFile<Project>(file);
        }

        public void Unload()
        {
            UnloadGameCodeDll();
            VisualStudio.CloseVisualStudio();
            UndoRedo.Reset();
        }


        public static void Save(Project project)
        {
            Serializer.ToFile(project, project.FullPath);
            Logger.Log(MessageType.Info, $"Project saved to {project.FullPath}");
        }

        private void SaveToBinary()
        {
            var configName = GetConfigurationName(StandaloneBuildConfig);
            var bin = $@"{Path}x64\{configName}\game.bin";

            using (var bw = new BinaryWriter(File.Open(bin, FileMode.Create, FileAccess.Write)))
            {
                if (ActiveScene != null)
                {
                    bw.Write(ActiveScene.GameEntities.Count);
                    foreach (var entity in ActiveScene.GameEntities)
                    {
                        bw.Write(0); // NOTE: entity type -> reserved for later
                        bw.Write(entity.Components.Count);
                        foreach(var component in entity.Components)
                        {
                            bw.Write((int)component.ToEnumType());
                            component.WriteToBinary(bw);
                        }
                    }
                }
            }
        }

        private async Task RunGame(bool debug)
        {
            string configName = GetConfigurationName(StandaloneBuildConfig);
            await Task.Run(() => VisualStudio.BuildSolution(this, configName, debug));
            if(VisualStudio.BuildSucceded)
            {
                SaveToBinary(); // so that we have up to date data in the "called" game
                await Task.Run(() => VisualStudio.Run(this, configName, debug));
            }
        }

        private async Task StopGame() => await Task.Run(() => VisualStudio.Stop());


        private async Task BuildGameCodeDll(bool showWindow = true)
        {
            try
            {
                UnloadGameCodeDll();
                await Task.Run(() => VisualStudio.BuildSolution(this, GetConfigurationName(DllBuildConfig), showWindow));
                if (VisualStudio.BuildSucceded)
                {
                    LoadGameCodeDll();
                }
            }
            catch(Exception ex)
            {
                Debug.WriteLine(ex.Message);
                throw;
            }
        }
        private void LoadGameCodeDll()
        {
            var configName = GetConfigurationName(DllBuildConfig);
            var dll = $@"{Path}x64\{configName}\{Name}.dll";

            AvailableScriptNames = null;

            if(File.Exists(dll) && EngineAPI.LoadGameCodeDll(dll) != 0)
            {
                AvailableScriptNames = EngineAPI.GetScriptNames();

                // remove entities before unloading game code dll (GameEntity.IsActive property adds game entity when being set to true)
                // NOTE: Change this
                ActiveScene?.GameEntities.Where(x => x.TryGetComponent<Script>() != null).ToList().ForEach(x => x.IsActive = true);
                Logger.Log(MessageType.Info, "Game code DLL loaded successfully.");
            }
            else
            {
                // just because if dll is already loaded and we haven't requested rebuild, in memory game_code_dll (EngineAPI\EngineAPI.cpp)
                // exists and LoadGameCodeDll that we dll import, will return FALSE (which is 0)
                Logger.Log(MessageType.Warning, "Failed to load game code DLL file. Try to build the project first.");
            }
        }

        private void UnloadGameCodeDll()
        {
            // remove entities before unloading game code dll (GameEntity.IsActive property removes game entity when being set to false)
            ActiveScene?.GameEntities.Where(x => x.TryGetComponent<Script>() != null).ToList().ForEach(x => x.IsActive = false);

            if(EngineAPI.UnloadGameCodeDll() != 0)
            {
                Logger.Log(MessageType.Info, "Game code DLL unloaded.");
                AvailableScriptNames = null;
            }
        }


        [OnDeserialized]
        private async void OnDeserialized(StreamingContext context)
        {
            //Initialize();
            if (_scenes != null)
            {
                Scenes = new ReadOnlyObservableCollection<Scene>(_scenes);
                OnPropertyChanged(nameof(Scenes)); // this makes the controls to update it's bindings to this list.
            }
            ActiveScene = Scenes.FirstOrDefault(x => x.IsActive);
            Debug.Assert(ActiveScene != null);

            // build game's code dll, but don't show a Visual Studio window.
            await BuildGameCodeDll(false);

            // because we await we gotta do it this way
            SetCommands();
        }

        public Project(string name, string path)
        {
            Name = name; 
            Path = path;

            // whenever project is created, it will have default scene
            //_scenes.Add(new Scene(this, "Default Scene"));
            OnDeserialized(new StreamingContext());
            
        }
    }
}
