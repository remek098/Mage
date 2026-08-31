using System;
using System.Collections.Generic;
using System.Text;
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
    /// Interaction logic for GeometryDetailView.xaml
    /// </summary>
    public partial class GeometryDetailView : UserControl
    {
        public GeometryDetailView()
        {
            InitializeComponent();
        }
        private void OnHighlight_CheckBox_Click(object sender, RoutedEventArgs e)
        {
            var vm = (GeometryEditor)DataContext;
            foreach(var m in vm.MeshRenderer.Meshes) {
                m.IsHighlighted = false;
            }

            var checkbox = sender as CheckBox;
            (checkbox?.DataContext as MeshRendererVertexData)?.IsHighlighted = checkbox?.IsChecked == true;
        }

        private void OnIsolate_CheckBox_Click(object sender, RoutedEventArgs e)
        {
            var vm = (GeometryEditor)DataContext;
            foreach (var m in vm.MeshRenderer.Meshes) {
                m.IsIsolated = false;
            }

            var checkbox = sender as CheckBox;
            var mesh = checkbox?.DataContext as MeshRendererVertexData;
            mesh?.IsIsolated = checkbox?.IsChecked == true;

            if(Tag is GeometryView geometryView) {
                geometryView.SetGeometry(mesh != null && mesh.IsIsolated ? vm.MeshRenderer.Meshes.IndexOf(mesh) : -1);
            }
        }

    }
}
