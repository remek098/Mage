using MageEditor.Components;
using MageEditor.GameProject;
using MageEditor.Utilities;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace MageEditor.Editors
{
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
            // go through all selected entities and remember the names
            var vm = DataContext as MSEntity;
            // register an action with a copy of these names of all selected entities as List
            var selection = vm?.SelectedEntities.Select(entity => (entity, entity.Name)).ToList();
            
            // undo action will change the name of each entity back to the copy we have saved.
            _undoAction = GetRenameAction();
        }

        private void OnName_TextBox_LostKeyboardFocus(object sender, KeyboardFocusChangedEventArgs e)
        {
            if(_propertyName == nameof(MSEntity.Name) && _undoAction != null)
            {
                var vm = DataContext as MSEntity;
                var selection = vm?.SelectedEntities.Select(entity => (entity, entity.Name)).ToList();
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
    }
}
