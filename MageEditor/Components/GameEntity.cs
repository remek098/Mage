using MageEditor.Common;
using MageEditor.DllWrappers;
using MageEditor.GameProject;
using MageEditor.Utilities;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;

namespace MageEditor.Components
{
    [DataContract]
    [KnownType(typeof(Transform))]
    [KnownType(typeof(Script))]
    class GameEntity : ViewModelBase
    {
        // engine side
        private int _entityID = ID.INVALID_ID;

        public int EntityID
        {
            get => _entityID;
            set
            {
                if (_entityID != value)
                {
                    _entityID = value;
                    OnPropertyChanged(nameof(EntityID));
                }
            }
        }

        // used to identify if GameEntity should be added or removed on the engine side
        private bool _isActive;
        public bool IsActive
        {
            get => _isActive;
            set
            {
                if (_isActive != value)
                {
                    _isActive = value;
                    if (_isActive)
                    {
                        // load entity to engine
                        EntityID = EngineAPI.EntityAPI.CreateGameEntity(this);
                        Debug.Assert(ID.IsValid(_entityID));
                    }
                    else if(ID.IsValid(EntityID))
                    {
                        // remove entity on engine side
                        EngineAPI.EntityAPI.RemoveGameEntity(this);
                        EntityID = ID.INVALID_ID;

                    }
                        OnPropertyChanged(nameof(IsActive));
                }
            }
        }

        // --------------------------------------------- editor side
        private bool _isEnabled = true;

        [DataMember]
        public bool IsEnabled
        {
            get => _isEnabled;
            set
            {
                if (_isEnabled != value)
                {
                    _isEnabled = value;
                    OnPropertyChanged(nameof(IsEnabled));
                }
            }
        }


        private string _name;

        [DataMember]
        public string Name
        {
            get => _name;
            set
            {
                if(_name != value)
                {
                    _name = value;
                    OnPropertyChanged(nameof(Name));
                }
            }
        }

        [DataMember]
        public Scene ParentScene { get; private set; }


        [DataMember(Name = nameof(Components))]
        private readonly ObservableCollection<Component> _components = new ObservableCollection<Component>();

        public ReadOnlyObservableCollection<Component> Components { get; private set; }

        // exact type -> gotta check if it's null though sometimes
        public Component? GetComponent(Type type) => Components.FirstOrDefault(c => c.GetType() == type);
        
        /// <summary>
        /// can include derived types -> say e.g. we want to get Transform, but we got another Component in our entity that also has Transform that derives from Component
        /// If component is optional (so essentially any component that is not Transform) use TryGetComponent instead
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <returns></returns>
        /// <exception cref="InvalidOperationException"></exception>
        public T GetComponent<T>() where T : Component => 
            Components.OfType<T>().FirstOrDefault()
                ?? throw new InvalidOperationException($"Component of type {typeof(T).Name} not found.");

        /// <summary>
        /// null if not the exact type; if exact type, then it's casted to it.
        /// Use this whenever component is not required to be in order for entity to exist (so anything other than Transform component)
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <returns></returns>
        public T? TryGetComponent<T>() where T : Component => GetComponent(typeof(T)) as T;
        //public ICommand RenameCommand { get; private set; }
        //public ICommand IsEnabledCommand { get; private set; }

        public bool AddComponent(Component component)
        {
            Debug.Assert(component != null);
            if(!Components.Any(x => x.GetType() == component.GetType()))
            {
                IsActive = false; // removes the entity from the engine -> invokes EngineAPI.EntityAPI.RemoveGameEntity()
                _components.Add(component);
                IsActive = true; // creates back new entity in engine -> invokes EngineAPI.EntityAPI.CreateGameEntity()
                return true;
            }
            Logger.Log(MessageType.Warning, $"Entity {Name} already has a {component.GetType().Name} component");
            return false;
        }

        public void RemoveComponent(Component component)
        {
            Debug.Assert(component != null);
            if (component is Transform) return; // we cannot and shouldn't ever remove Transform component

            if(Components.Contains(component))
            {
                IsActive = false; // removes the entity from  the engine
                _components.Remove(component);
                IsActive = true; // adds it back to the engine
            }
        }

        [OnDeserialized]
        void OnDeserialized(StreamingContext context)
        {
            if(_components != null)
            {
                Components = new ReadOnlyObservableCollection<Component>(_components);
                OnPropertyChanged(nameof(Components));
            }
            
        }

        public GameEntity(Scene scene)
        {
            Debug.Assert(scene != null);
            ParentScene = scene;
            _components.Add(new Transform(this));
            OnDeserialized(new StreamingContext());
        }
    }

    abstract class MSEntity : ViewModelBase
    {
        // enables updates to selected entities.
        private bool _enableUpdates = true;


        // this value will be null to indicate that multiply-selected entities have diffrent value of IsEnabled field
        private bool? _isEnabled = true;
        public bool? IsEnabled
        {
            get => _isEnabled;
            set
            {
                if (_isEnabled != value)
                {
                    _isEnabled = value;
                    OnPropertyChanged(nameof(IsEnabled));
                }
            }
        }

        // string is a reference type
        private string? _name;
        public string? Name
        {
            get => _name;
            set
            {
                if (_name != value)
                {
                    _name = value;
                    OnPropertyChanged(nameof(Name));
                }
            }
        }


        // using IMSComponent interface so that we can support multiple components
        private readonly ObservableCollection<IMSComponent> _components = new ObservableCollection<IMSComponent>();
        public ReadOnlyObservableCollection<IMSComponent> Components { get; }

        public T? GetMSComponent<T>() where T : IMSComponent
        {
            return (T?)Components.FirstOrDefault(x => x.GetType() == typeof(T));
        }



        public List<GameEntity> SelectedEntities { get; }

        private void MakeComponentList()
        {
            _components.Clear();
            var firstEntity = SelectedEntities.FirstOrDefault();
            if (firstEntity == null) return;

            // in the first game entity that we got during multiselection, we get first component, we get its type
            foreach ( var component in firstEntity.Components )
            {
                var type = component.GetType();
                // if all other entities have the same component type, then we will be adding component to the list of multi-selected components
                if(!SelectedEntities.Skip(1).Any(entity => entity.GetComponent(type) == null))
                {
                    Debug.Assert(Components.FirstOrDefault(x => x.GetType() == type) == null);
                    _components.Add(component.GetMultiSelectionComponent(this));
                }
            }
        }


        //public static float? GetMixedValue(List<GameEntity> entities, Func<GameEntity, float> getProperty)
        //{
        //    // if that's the only entity we're done, if more we compare the other values to the first one. If we can find entity that has other value, we have non-uniform value
        //    // and we got to return null. Otherwise we have uniform value and we can return it.
        //    var value = getProperty(entities.First());
        //    foreach (var entity in entities.Skip(1))
        //    {
        //        if(!value.IsTheSameAs(getProperty(entity)))
        //        {
        //            return null;
        //        }
        //    }
        //    return value;
        //}

        public static float? GetMixedValue<T>(List<T> objects, Func<T, float> getProperty)
        {
            var value = getProperty(objects.First());
            // skipping first object since we obtain value
            // if  the value we get is diffrent than what we got from the first object, then it means we got a non-uniform value during multi-selection
            return objects.Skip(1).Any(x => !getProperty(x).IsTheSameAs(value)) ? (float?)null : value;
        }

        public static bool? GetMixedValue<T>(List<T> objects, Func<T, bool> getProperty)
        {
            var value = getProperty(objects.First());
            // skipping first object since we obtain value
            // if  the value we get is diffrent than what we got from the first object, then it means we got a non-uniform value during multi-selection
            return objects.Skip(1).Any(x => value != getProperty(x)) ? (bool?)null : value;
        }

        public static string? GetMixedValue<T>(List<T> objects, Func<T, string> getProperty)
        {
            var value = getProperty(objects.First());
            // skipping first object since we obtain value
            // if  the value we get is diffrent than what we got from the first object, then it means we got a non-uniform value during multi-selection
            return objects.Skip(1).Any(x => value != getProperty(x)) ? null : value;
        }

        // ofc other entities will inherit it and can be free to modify this function if needed
        protected virtual bool UpdateGameEntities(string propertyName)
        {
            switch (propertyName)
            {
                case nameof(IsEnabled): SelectedEntities.ForEach(x => { if (IsEnabled is not null) x.IsEnabled = IsEnabled.Value; }); return true;
                case nameof(Name): SelectedEntities.ForEach(x => { if(Name is not null) x.Name = Name; }); return true;
            }
            return false;
        }


        // calling this means that it will also call UpdateGameEntities() of each selected element and update it's properties.
        protected virtual bool UpdateMSGameEntity()
        {
            // go through all selected entities and use this function to get their property that we want to have
            IsEnabled = GetMixedValue(SelectedEntities, new Func<GameEntity, bool>(x => x.IsEnabled));
            Name = GetMixedValue(SelectedEntities, new Func<GameEntity, string>(x => x.Name));

            return true;
        }

        public void Refresh()
        {
            _enableUpdates = false;
            UpdateMSGameEntity();
            MakeComponentList();
            _enableUpdates = true;
        }

        public MSEntity(List<GameEntity> entities)
        {
            Debug.Assert(entities?.Any() == true);
            Components = new ReadOnlyObservableCollection<IMSComponent>(_components);
            SelectedEntities = entities;
            
            PropertyChanged += (s, e) => 
            { 
                Debug.Assert(e.PropertyName != null); 
                if(_enableUpdates) UpdateGameEntities(e.PropertyName);

            };
        }
    }

    class MSGameEntity : MSEntity
    {
        public MSGameEntity(List<GameEntity> entities) : base(entities)
        {
            // fetches all the data from selected entities
            Refresh();
        }
    }
}
