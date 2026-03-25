using MageEditor.Components;
using MageEditor.EngineAPIStructs;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace MageEditor.EngineAPIStructs
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
    class GameEntityDescriptor
    {
        // it has to match GameEntityDesc from EngineDLL project (in /EngineDLL/EngineAPI.cpp file)
        public TransformComponent Transform = new TransformComponent();
    }
}


namespace MageEditor.DllWrappers
{
    static class EngineAPI
    {
        private const string _dllName = "EngineDLL.dll";

        [DllImport(_dllName)]
        private static extern int CreateGameEntity(GameEntityDescriptor desc);

        // this function will convert editors GameEntity into GameEntityDescriptor which will be matching engine's side of things.
        public static int CreateGameEntity(GameEntity entity)
        {
            GameEntityDescriptor desc = new GameEntityDescriptor();

            // transform component
            {
                var component = entity.GetComponent<Transform>();
                desc.Transform.Position = component.Position;
                desc.Transform.Rotation = component.Rotation;
                desc.Transform.Scale = component.Scale;
            }

            return CreateGameEntity(desc);
        }


        [DllImport(_dllName)]
        private static extern void RemoveGameEntity(int id);
        public static void RemoveGameEntity(GameEntity entity)
        {
            RemoveGameEntity(entity.EntityID);
        }
    }
}
