using MageEditor.ContentToolsAPIStructs;
using MageEditor.DllWrappers;
using MageEditor.Editors;
using MageEditor.Utilities.Controls;
using System;
using System.Collections.Generic;
using System.IO;
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
using System.Windows.Shapes;

namespace MageEditor.Content
{
    /// <summary>
    /// Interaction logic for PrimitiveMeshDialog.xaml
    /// </summary>
    public partial class PrimitiveMeshDialog : Window
    {
       private static readonly List<ImageBrush> _textures = new List<ImageBrush>();

        private void OnPrimitiveType_ComboBox_SelectionChangeed(object sender, SelectionChangedEventArgs e) => UpdatePrimitive();

        private void OnSlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e) => UpdatePrimitive();
        private void OnScalarBox_ValueChanged(object sender, RoutedEventArgs e) => UpdatePrimitive();

        private float ParseValueToFloat(ScalarBox scalarBox, float min)
        {
            float.TryParse(scalarBox.Value, out var result);
            return Math.Max(result, min);
        }

        private void UpdatePrimitive()
        {
            // ComboBox's function will be called on initialization, we gotta check if we're initialized before doing anything
            if (!IsInitialized) return;

            var primitiveType = (PrimitiveMeshType)primitiveTypeComboBox.SelectedItem;
            var info = new PrimitiveInitInfo() { Type = primitiveType };
            var smoothingAngle = 0;

            switch (primitiveType)
            {
                case PrimitiveMeshType.Plane:
                    {
                        // plane doesn't need LOD setting for now
                        info.SegmentX = (int)xSliderPlane.Value;
                        info.SegmentZ = (int)zSliderPlane.Value;
                        // imagine basic unit is 1m, therefore we want 1mm by 1mm as the smallest possible plane
                        info.Size.X = ParseValueToFloat(widthScalarBoxPlane, 0.001f);
                        info.Size.Z = ParseValueToFloat(lengthScalarBoxPlane, 0.001f);
                    }
                    break;
                case PrimitiveMeshType.Cube:
                    return;
                case PrimitiveMeshType.UvSphere:
                    {
                        info.SegmentX = (int)xSliderUvSphere.Value;
                        info.SegmentY = (int)ySliderUvSphere.Value;
                        info.Size.X = ParseValueToFloat(xScalarBoxUvSphere, 0.001f);
                        info.Size.Y = ParseValueToFloat(yScalarBoxUvSphere, 0.001f);
                        info.Size.Z = ParseValueToFloat(zScalarBoxUvSphere, 0.001f);
                        smoothingAngle = (int)angleSliderUvSphere.Value;
                    }
                    break;
                case PrimitiveMeshType.IcoSphere:
                    return;
                case PrimitiveMeshType.Cylinder:
                    return;
                case PrimitiveMeshType.Capsule:
                    return;

                default:
                    break;
            }

            var geometry = new Geometry();
            geometry.ImportSettings.SmoothingAngle = smoothingAngle;
            ContentToolsAPI.CreatePrimitiveMesh(geometry, info);
            // we got GeometryEditor in PrimitiveMeshDialog's Window.DataContext
            (DataContext as GeometryEditor)?.SetAsset(geometry); // render this geometry using WPF's 3d renderer
            OnTexture_CheckBox_Click(textureCheckBox, null);
        }

        private static void LoadTextures()
        {
            var uris = new List<Uri>
            {
                new Uri("pack://application:,,,/Resources/PrimitiveMeshView/PlaneTexture.png"),
                new Uri("pack://application:,,,/Resources/PrimitiveMeshView/PlaneTexture.png"),
                new Uri("pack://application:,,,/Resources/PrimitiveMeshView/CheckerMap.png"),
            };

            _textures.Clear();
            foreach(var uri in uris)
            {
                var resource = Application.GetResourceStream(uri);
                using var reader = new BinaryReader(resource.Stream);
                var data = reader.ReadBytes((int)resource.Stream.Length);
                // just because data is nullable, we gotta obey C# laws in .NET 9.0 :(
                var imageSource = (BitmapSource?)new ImageSourceConverter().ConvertFrom(data);
                imageSource?.Freeze();
                var brush = new ImageBrush(imageSource);
                // uv directions in WPF are in the opposite direction, therefore we have to flip v direction
                // uses center of the texture image to do that
                brush.Transform = new ScaleTransform(1, -1, 0.5, 0.5);
                // avoid renderer rescaling units to be always in [0,1]
                brush.ViewportUnits = BrushMappingMode.Absolute;
                brush.Freeze();
                _textures.Add(brush);
            }
        }

        static PrimitiveMeshDialog()
        {
            LoadTextures();
        }

        public PrimitiveMeshDialog()
        {
            InitializeComponent();
            Loaded += (s, e) => UpdatePrimitive(); // make sure we have primitive already waiting when we're being redirected to this dialog
        }

        private void OnTexture_CheckBox_Click(object sender, RoutedEventArgs? e)
        {
            Brush brush = Brushes.White;
            if((sender as CheckBox)?.IsChecked == true)
            {
                brush = _textures[(int)primitiveTypeComboBox.SelectedIndex];
            }

            var ge = (GeometryEditor)DataContext;
            foreach(var mesh in ge.MeshRenderer.Meshes)
            {
                mesh.Diffuse = brush;
            }
        }
    }
}
