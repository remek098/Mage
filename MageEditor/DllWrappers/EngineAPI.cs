using MageEditor.Components;
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
                    //var c = entity.GetComponent<Script>();
                }

                return CreateGameEntity(desc);
            }


            [DllImport(_engineDll)]
            private static extern void RemoveGameEntity(int id);
            public static void RemoveGameEntity(GameEntity entity)
            {
                RemoveGameEntity(entity.EntityID);
            }
        }
    }
}
