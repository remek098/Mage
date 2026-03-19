using MageEditor.Components;
using MageEditor.Editors;
using MageEditor.GameProject;
using MageEditor.Utilities;
using System;
using System.Collections.Generic;
using System.Diagnostics;
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
    /// Interaction logic for ProjectLayoutView.xaml
    /// </summary>
    public partial class ProjectLayoutView : UserControl
    {
        public ProjectLayoutView()
        {
            InitializeComponent();
            // DataContext = Project.Current;
            Loaded += (_, __) => Debug.WriteLine(DataContext?.GetType().Name);
        }

        private void OnAddGameEntity_Button_Click(object sender, RoutedEventArgs e)
        {
            var btn = sender as Button;
            var vm = btn?.DataContext as Scene;
            vm?.AddGameEntityCommand.Execute(new GameEntity(vm) { Name = "Empty Game Entity"});
        }

        private void OnGameEntities_ListBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            //GameEntityView.Instance.DataContext = null; // not needed since now msEntity would be just null if nothing was selected
            var listBox = sender as ListBox;
            
            var newSelection = listBox?.SelectedItems.Cast<GameEntity>().ToList();
            var previousSelection = newSelection?.Except(e.AddedItems.Cast<GameEntity>()).Concat(e.RemovedItems.Cast<GameEntity>()).ToList();

            
            Project.UndoRedo.Add(new UndoRedoAction(
                () => // undo action
                {
                    listBox?.UnselectAll();
                    previousSelection?.ForEach(x => (listBox?.ItemContainerGenerator.ContainerFromItem(x) as ListBoxItem)?.SetValue(ListBoxItem.IsSelectedProperty, true));
                },
                () => // redo action
                {
                    listBox?.UnselectAll();
                    newSelection?.ForEach(x => (listBox?.ItemContainerGenerator.ContainerFromItem(x) as ListBoxItem)?.SetValue(ListBoxItem.IsSelectedProperty, true));
                },
                "Selection changed"
            ));


            MSGameEntity msEntity = null;
            if(newSelection != null &&  newSelection.Any())
            {
                msEntity = new MSGameEntity(newSelection);
            }
            GameEntityView.Instance.DataContext = msEntity;
        }
    }
}
