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
using System.Windows.Media.Media3D;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace MageEditor.Editors
{
    /// <summary>
    /// Interaction logic for GeometryView.xaml
    /// </summary>
    public partial class GeometryView : UserControl
    {
        private static readonly GeometryView _geometryView = new GeometryView() { Background = (Brush)Application.Current.FindResource("Editor.Window.GrayBrush4") };
        private Point _clickedPosition;
        private bool _capturedLeft;
        private bool _capturedRight;


        /// <summary>
        /// 
        /// </summary>
        /// <param name="index">If you want to display all meshes, index should be -1 (default), otherwise it will chose a mesh by index to display.</param>
        public void SetGeometry(int index = -1)
        {
            if (!(DataContext is MeshRenderer mr)) return;

            // viewport is our Viewport3D we have in our GeometryView.xaml
            // the first child we added is a collection of light (at 0)
            // if viewport.Children.Count is 2, we for sure have Geometry being displayed
            if(mr.Meshes.Any() && viewport.Children.Count == 2)
            {
                // whenever we want to set new mesh to display, we have to remove old geometry
                viewport.Children.RemoveAt(1);
            }

            var meshIndex = 0;
            var modelGroup = new Model3DGroup();
            foreach(var mesh in mr.Meshes)
            {
                // skip over meshes that we don't want to display.
                if(index != -1 && meshIndex != index)
                {
                    ++meshIndex;
                    continue;
                }

                var mesh3D = new MeshGeometry3D()
                {
                    Positions = mesh.Positions,
                    Normals = mesh.Normals,
                    TriangleIndices = mesh.Indices,
                    TextureCoordinates = mesh.UVs
                };

                var diffuse = new DiffuseMaterial(mesh.Diffuse);
                var specular = new SpecularMaterial(mesh.Specular, 50); // the higher the number, the smaller the specular highlight will be
                var matGroup = new MaterialGroup();
                matGroup.Children.Add(diffuse);
                matGroup.Children.Add(specular);

                var model = new GeometryModel3D(mesh3D, matGroup);
                modelGroup.Children.Add(model); // just like for lights in GeometryViewer.xaml, we got Model3DGroup for meshes.

                // set binding, so that whenever we change Diffuse property, in viewer it also updates.
                var binding = new Binding(nameof(mesh.Diffuse)){ Source = mesh };
                BindingOperations.SetBinding(diffuse, DiffuseMaterial.BrushProperty, binding);

                if (meshIndex == index) break; // if that's a mesh we wanted to display, stop processing.
            }

            // just like for lighting, to see the meshes in our View, we got to do this.
            var visual = new ModelVisual3D() { Content = modelGroup };
            viewport.Children.Add(visual);
        }

        private void OnGrid_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            _clickedPosition = e.GetPosition(this);
            _capturedLeft = true;
            Mouse.Capture(sender as UIElement);
        }

        private void OnGrid_MouseMove(object sender, MouseEventArgs e)
        {
            if(!_capturedLeft && !_capturedRight) return;

            var pos = e.GetPosition(this);
            var d = pos - _clickedPosition;

            if(_capturedLeft && !_capturedRight)
            {
                // move camera around the object
                MoveCamera(d.X, d.Y, 0);
            }
            else if(!_capturedLeft && _capturedRight)
            {
                // change the location camera is looking at in vertical direction
                var mr = (MeshRenderer)DataContext; // not doing DataContext as MeshRenderer, to avoid dealing with extra if statement
                var cp = mr.CameraPosition;
                // 1 pixel of movement should be 1mm in our arbitrary world units; Sqrt makes sure that if we're far from the object
                // the camera movement is scaled correspondingly
                var yOffset = d.Y * 0.001 * Math.Sqrt(cp.X * cp.X + cp.Z * cp.Z);
                mr.CameraTarget = new Point3D(mr.CameraTarget.X, mr.CameraTarget.Y + yOffset, mr.CameraTarget.Z);
            }

            _clickedPosition = pos;
        }

        private void OnGrid_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            _capturedLeft = false;
            if(!_capturedRight) Mouse.Capture(null);
        }

        private void OnGrid_MouseWheel(object sender, MouseWheelEventArgs e)
        {
            // change distance between object and camera
            MoveCamera(0, 0, Math.Sign(e.Delta));
        }

        private void OnGrid_MouseRightButtonDown(object sender, MouseButtonEventArgs e)
        {
            _clickedPosition = e.GetPosition(this);
            _capturedRight = true;
            Mouse.Capture(sender as UIElement);
        }

        private void OnGrid_MouseRightButtonUp(object sender, MouseButtonEventArgs e)
        {
            _capturedRight = false;
            if(!_capturedLeft) Mouse.Capture(null);
        }

        private void MoveCamera(double dx, double dy, int dz)
        {
            var mr = (MeshRenderer)DataContext;
            var v = new Vector3D(mr.CameraPosition.X, mr.CameraPosition.Y, mr.CameraPosition.Z);

            // spherical coordinates using camera position
            var r = v.Length;
            var theta = Math.Acos(v.Y / r);
            var phi = Math.Atan2(-v.Z, v.X);

            // scaling the pixels diffrence by a proper value so it ain't too fast
            theta -= dy * 0.01;
            phi -= dx * 0.01;
            r *= 1.0 - 0.1 * dz; // dx is either +1 or -1

            // clamping to values close to 0 and Pi, because when 0 or Pi the camera's up vector is undefined
            theta = Math.Clamp(theta, 0.0001, Math.PI - 0.0001);

            // convert from spherical to cartesian coordinates
            v.X = r * Math.Sin(theta) * Math.Cos(phi); // might be sin*cos (probably not)
            v.Z = -r * Math.Sin(theta) * Math.Sin(phi);
            v.Y = r * Math.Cos(theta);

            mr.CameraPosition = new Point3D(v.X, v.Y, v.Z);
        }

        internal static BitmapSource RenderToBitmap(MeshRenderer meshRenderer, int width, int height)
        {
            var bmp = new RenderTargetBitmap(width, height, 96, 96, PixelFormats.Default);

            // we're not creating instance of this GeometryView when we try to render it.
            // we need static instance to be ready for this operation
            _geometryView.DataContext = meshRenderer;
            _geometryView.Width = width;
            _geometryView.Height = height;
            _geometryView.Measure(new Size(width, height));
            _geometryView.Arrange(new Rect(0, 0, width, height));
            _geometryView.UpdateLayout();

            bmp.Render(_geometryView);
            return bmp;
        }

        public GeometryView()
        {
            InitializeComponent();
            DataContextChanged += (s, e) => SetGeometry();
        }
    }
}
