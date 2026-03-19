using MageEditor.Components;
using MageEditor.Utilities;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Controls;
using System.Windows.Input;

namespace MageEditor.GameProject
{
    [DataContract]
    class Scene : ViewModelBase
    {
        
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
        public Project Project { get; private set; }


        private bool _isActive;
        
        [DataMember]
        public bool IsActive
        {
            get => _isActive;
            set
            {
                if (_isActive != value)
                {
                    _isActive = value;
                    OnPropertyChanged(nameof(IsActive));
                }
            }
        }

        [DataMember(Name = nameof(GameEntities))]
        private readonly ObservableCollection<GameEntity> _gameEntities = [];
        public ReadOnlyObservableCollection<GameEntity> GameEntities { get; private set; }

        public ICommand AddGameEntityCommand { get; private set; }
        public ICommand RemoveGameEntityCommand { get; private set; }

        private void AddGameEntity(GameEntity entity)
        {
            Debug.Assert(!_gameEntities.Contains(entity));
            _gameEntities.Add(entity);
        }

        private void RemoveGameEntity(GameEntity entity)
        {
            Debug.Assert(_gameEntities.Contains(entity));
            _gameEntities.Remove(entity);
        }

        [OnDeserialized]
        private void OnDeserialized(StreamingContext context)
        {
            //if (_gameEntities == null) _gameEntities = new ObservableCollection<GameEntity>();
         
            if (_gameEntities != null)
            {
                GameEntities = new ReadOnlyObservableCollection<GameEntity>(_gameEntities);
                OnPropertyChanged(nameof(GameEntities)); // this makes the controls to update it's bindings to this list.
            }

            AddGameEntityCommand = new RelayCommand<GameEntity>(x =>
            {
                AddGameEntity(x);
                var entityIndex = _gameEntities?.Count - 1;
                
                if(entityIndex.HasValue)
                    Project.UndoRedo.Add(new UndoRedoAction(
                        () => RemoveGameEntity(x),
                        () => _gameEntities?.Insert(entityIndex.Value, x),
                        $"Add {x?.Name} to {Name}"
                    ));

            });

            RemoveGameEntityCommand = new RelayCommand<GameEntity>(x =>
                {
                    var entityIndex = _gameEntities?.IndexOf(x);
                    RemoveGameEntity(x);

                    if (entityIndex.HasValue)
                        Project.UndoRedo.Add(new UndoRedoAction(
                            () => _gameEntities?.Insert(entityIndex.Value, x),
                            () => RemoveGameEntity(x),
                            $"Remove {x.Name}"
                        ));
                }
            );


        }

        public Scene(Project project, string name)
        {
            Debug.Assert(project != null);
            Project = project;
            Name = name;
            //OnDeserialized(new StreamingContext());
        }

    }
}
