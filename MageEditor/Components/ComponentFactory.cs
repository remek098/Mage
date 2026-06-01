using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MageEditor.Components
{
    enum ComponentType
    {
        Transform,
        Script,
    }

    static class ComponentFactory
    {
        private static readonly Func<GameEntity, object, Component>[] _functions =
            new Func<GameEntity, object, Component>[]
            {
                (entity, data) => new Transform(entity),
                (entity, data) => new Script(entity){ Name = (string)data},
            };
        public static Func<GameEntity, object, Component> GetCreationFunction(ComponentType comp_type)
        {
            Debug.Assert((int)comp_type < _functions.Length);
            return _functions[(int)comp_type];
        }

        public static ComponentType ToEnumType(this Component component)
        {
            return component switch
            {
                Transform _ => ComponentType.Transform,
                Script _ => ComponentType.Script,
                _ => throw new ArgumentException("Unknown component type."),
            };
        }
    }
}
