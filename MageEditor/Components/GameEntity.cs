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
                        EntityID = EngineAPI.CreateGameEntity(this);
                        Debug.Assert(ID.IsValid(_entityID));
                    }
                    else
                    {
                        // remove entity on engine side
                        EngineAPI.RemoveGameEntity(this);

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


        public Component GetComponent(Type type) => Components.FirstOrDefault(c => c.GetType() == type);
        public T GetComponent<T>() where T : Component => GetComponent(typeof(T)) as T;
        //public ICommand RenameCommand { get; private set; }
        //public ICommand IsEnabledCommand { get; private set; }

        [OnDeserialized]
        void OnDeserialized(StreamingContext context)
        {
            if(_components != null)
            {
                Components = new ReadOnlyObservableCollection<Component>(_components);
                OnPropertyChanged(nameof(Components));
            }

            // can't use these because if we did, we would have to undo e.g. 10 times if we changed a name of multiply selected entities.
            //RenameCommand = new RelayCommand<string>(x =>
            //{
            //    var oldName = _name;
            //    Name = x;
            //    Project.UndoRedo.Add(new UndoRedoAction(nameof(Name), this, oldName, x, $"Renamed entity '{oldName}' to '{x}'"));
            //}, x => x!= _name); // we can change name only if new name is diffrent than current name.

            //IsEnabledCommand = new RelayCommand<bool>(x =>
            //{
            //    var oldName = _isEnabled;
            //    IsEnabled = x;
            //    Project.UndoRedo.Add(new UndoRedoAction(nameof(IsEnabled), this, oldName, x, x? $"Enabled {Name}" : $"Disabled {Name}"));
            //});


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
        private string _name;
        public string Name
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

        public List<GameEntity> SelectedEntities { get; }
        

        public static float? GetMixedValue(List<GameEntity> entities, Func<GameEntity, float> getProperty)
        {
            // if that's the only entity we're done, if more we compare the other values to the first one. If we can find entity that has other value, we have non-uniform value
            // and we got to return null. Otherwise we have uniform value and we can return it.
            var value = getProperty(entities.First());
            foreach (var entity in entities.Skip(1))
            {
                if(!value.IsTheSameAs(getProperty(entity)))
                {
                    return null;
                }
            }
            return value;
        }

        public static bool? GetMixedValue(List<GameEntity> entities, Func<GameEntity, bool> getProperty)
        {
            // if that's the only entity we're done, if more we compare the other values to the first one. If we can find entity that has other value, we have non-uniform value
            // and we got to return null. Otherwise we have uniform value and we can return it.
            var value = getProperty(entities.First());
            foreach (var entity in entities.Skip(1))
            {
                if (value != getProperty(entity))
                {
                    return null;
                }
            }
            return value;
        }

        public static string GetMixedValue(List<GameEntity> entities, Func<GameEntity, string> getProperty)
        {
            // if that's the only entity we're done, if more we compare the other values to the first one. If we can find entity that has other value, we have non-uniform value
            // and we got to return null. Otherwise we have uniform value and we can return it.
            var value = getProperty(entities.First());
            foreach (var entity in entities.Skip(1))
            {
                if (value != getProperty(entity))
                {
                    return null;
                }
            }
            return value;
        }

        // ofc other entities will inherit it and can be free to modify this function if needed
        protected virtual bool UpdateGameEntities(string propertyName)
        {
            switch (propertyName)
            {
                case nameof(IsEnabled): SelectedEntities.ForEach(x => x.IsEnabled = IsEnabled.Value); return true;
                case nameof(Name): SelectedEntities.ForEach(x => x.Name = Name); return true;
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
