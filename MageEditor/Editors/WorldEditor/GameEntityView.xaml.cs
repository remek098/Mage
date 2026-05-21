using MageEditor.Components;
using MageEditor.GameProject;
using MageEditor.Utilities;
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace MageEditor.Editors
{
    public class NullableBoolToBoolConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            return value is bool b && b == true;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            return value is bool b && b == true;
        }
    }

    /// <summary>
    /// Interaction logic for GameEntityView.xaml
    /// </summary>
    public partial class GameEntityView : UserControl
    {
        private Action? _undoAction;
        private string? _propertyName;

        public static GameEntityView Instance { get; private set; }
        public GameEntityView()
        {
            InitializeComponent();
            DataContext = null;
            Instance = this;

            DataContextChanged += (_, __) =>
            {
                if (DataContext != null)
                {
                    ((MSEntity)DataContext).PropertyChanged += (s, e) => _propertyName = e.PropertyName;
                }
            };
        }

        private Action GetRenameAction()
        {
            // go through all selected entities and remember the names
            var vm = DataContext as MSEntity;
            // register an action with a copy of these names of all selected entities as List
            var selection = vm?.SelectedEntities.Select(entity => (entity, entity.Name)).ToList();

            // undo action will change the name of each entity back to the copy we have saved.
            return new Action(() =>
            {
                selection?.ForEach(item => item.entity.Name = item.Name);
                (DataContext as MSEntity)?.Refresh(); // fetch back old names again
            }); 
        }

        private Action GetIsEnabledAction()
        {
            // go through all selected entities and remember the names
            var vm = DataContext as MSEntity;
            // register an action with a copy of these names of all selected entities as List
            var selection = vm?.SelectedEntities.Select(entity => (entity, entity.IsEnabled)).ToList();

            // undo action will change the name of each entity back to the copy we have saved.
            return new Action(() =>
            {
                selection?.ForEach(item => item.entity.IsEnabled = item.IsEnabled);
                (DataContext as MSEntity)?.Refresh(); // fetch back old names again
            });
        }

        private void OnName_TextBox_GotKeyboardFocus(object sender, KeyboardFocusChangedEventArgs e)
        {
            _propertyName = string.Empty;
            // undo action will change the name of each entity back to the copy we have saved.
            _undoAction = GetRenameAction();
        }

        private void OnName_TextBox_LostKeyboardFocus(object sender, KeyboardFocusChangedEventArgs e)
        {
            if(_propertyName == nameof(MSEntity.Name) && _undoAction != null)
            {
                var redoAction = GetRenameAction();
                Project.UndoRedo.Add(new UndoRedoAction(_undoAction, redoAction, "Rename game entity"));
                _propertyName = null;
            }
            _undoAction = null;
        }

        private void OnIsEnabled_CheckBox_Click(object sender, RoutedEventArgs e)
        {
            var undoAction = GetIsEnabledAction();
            var vm = (MSEntity)DataContext;
            vm.IsEnabled = (sender as CheckBox)?.IsChecked == true;

            var redoAction = GetIsEnabledAction();
            Project.UndoRedo.Add(new UndoRedoAction(undoAction, redoAction,
                vm?.IsEnabled == true ? "Enable game entity" : "Disable game entity"));
        }

        private void OnAddComponent_Button_PreviewMouse_LMB_Down(object sender, MouseButtonEventArgs e)
        {
            var menu = FindResource("addComponentMenu") as ContextMenu;
            var btn = sender as ToggleButton;
            if (btn != null && menu != null)
            {
                menu.Placement = PlacementMode.Bottom;
                btn.IsChecked = true;
                menu.PlacementTarget = btn;
                menu.MinWidth = btn.ActualWidth;
                menu.IsOpen = true;
            }
        }

        private void AddComponent(ComponentType comp_type, object data)
        {
            var creationFunction = ComponentFactory.GetCreationFunction(comp_type);
            var changedEntities = new List<(GameEntity entity, Component component)>(); // for undo/redo actions
            var vm = (MSEntity)DataContext;

            // try to add this component to each selected entity
            foreach(var entity in vm.SelectedEntities)
            {
                var component = creationFunction(entity, data); // create component of requested type
                if (entity.AddComponent(component))
                {
                    changedEntities.Add((entity, component));
                }
            }

            if(changedEntities.Any())
            {
                vm.Refresh();
                Project.UndoRedo.Add(new UndoRedoAction(
                    () =>
                    {
                        changedEntities.ForEach(x => x.entity.RemoveComponent(x.component));
                        (DataContext as MSEntity)?.Refresh();
                    },
                    () =>
                    {
                        changedEntities.ForEach(x => x.entity.AddComponent(x.component));
                        (DataContext as MSEntity)?.Refresh();
                    },
                    $"Add {comp_type} component"
                ));
            } 
        }

        private void OnAddScriptComponent(object sender, RoutedEventArgs e)
        {
            var header_string = (sender as MenuItem)?.Header.ToString();
            if(header_string != null) AddComponent(ComponentType.Script, header_string);
        }
    }
}
