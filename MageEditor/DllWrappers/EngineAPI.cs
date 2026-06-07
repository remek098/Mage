using MageEditor.Components;
using MageEditor.GameProject;
using MageEditor.Utilities;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace MageEditor.DllWrappers
{
    // define all appropriate structs the same way as they've been defined inside EngineDLL project.
    [StructLayout(LayoutKind.Sequential)]
    class TransformComponent
    {
        public Vector3 Position;
        public Vector3 Rotation;
        public Vector3 Scale = new Vector3(1, 1, 1);
    }

    [StructLayout(LayoutKind.Sequential)]
    class ScriptComponent
    {
        public IntPtr ScriptCreator;
    }

    [StructLayout(LayoutKind.Sequential)]
    class GameEntityDescriptor
    {
        // it has to match GameEntityDesc from EngineDLL project (in /EngineDLL/EngineAPI.cpp file)
        public TransformComponent Transform = new TransformComponent();
        public ScriptComponent Script = new ScriptComponent();
    }
}


namespace MageEditor.DllWrappers
{
    static class EngineAPI
    {
        // NOTE: using int return type for matching u32 (unsigned 32-bit int) return type from EngineDll functions
        // instead of unsigned int (u32), because it doesn't make any diffrence for bit interpretation in this case
        private const string _engineDll = "EngineDLL.dll";

        [DllImport(_engineDll, CharSet = CharSet.Ansi)]
        public static extern int LoadGameCodeDll(string dllPath);

        [DllImport(_engineDll)]
        public static extern int UnloadGameCodeDll();

        // NOTE: editor needs to pass this information from GameCode to EngineDll
        [DllImport(_engineDll)]
        public static extern IntPtr GetScriptCreator(string name);

        [DllImport(_engineDll)]
        [return: MarshalAs(UnmanagedType.SafeArray)]
        public static extern string[] GetScriptNames();


        // ----------------------------- Render surfaces
        
        [DllImport(_engineDll)]
        public static extern int CreateRenderSurface(IntPtr host, int width, int height);
        [DllImport(_engineDll)]
        public static extern void RemoveRenderSurface(int surfaceId);

        /// <summary>
        /// Used because of Data-Oriented Design for window management
        /// </summary>
        /// <param name="surfaceId"></param>
        /// <returns></returns>
        [DllImport(_engineDll)]
        public static extern IntPtr GetWindowHandle(int surfaceId);


        // -------------------------- END Render surfaces

        internal static class EntityAPI
        {
            [DllImport(_engineDll)]
            private static extern int CreateGameEntity(GameEntityDescriptor desc);

            // this function will convert editors GameEntity into GameEntityDescriptor which will be matching engine's side of things.
            public static int CreateGameEntity(GameEntity entity)
            {
                GameEntityDescriptor desc = new GameEntityDescriptor();

                // transform component
                {
                    var component = entity.GetComponent<Transform>();
                    //if(component is not null)
                    //{
                    //}
                    desc.Transform.Position = component.Position;
                    desc.Transform.Rotation = component.Rotation;
                    desc.Transform.Scale = component.Scale;
                }

                // script component
                {
                    var c = entity.TryGetComponent<Script>();
                    if (c != null && Project.Current != null)
                    {
                        // available script names might be null e.g. when loading/reloading game code dll. -> Project.LoadGameCodeDll()
                        if (Project.Current.AvailableScriptNames != null && Project.Current.AvailableScriptNames.Contains(c.Name))
                        {
                            desc.Script.ScriptCreator = GetScriptCreator(c.Name);
                        }
                        else
                        {
                            Logger.Log(MessageType.Error, $"Unable to find script with name {c.Name}. Game entity will be created without script component!");
                        }
                    }
                }

                // Logger.Log(MessageType.Info, $"Creating entity {entity.Name}");
                return CreateGameEntity(desc);
            }


            [DllImport(_engineDll)]
            private static extern void RemoveGameEntity(int id);
            public static void RemoveGameEntity(GameEntity entity)
            {
                // Logger.Log(MessageType.Info, $"Removing entity {entity.Name}");
                RemoveGameEntity(entity.EntityID);
            }
        }
    }
}
