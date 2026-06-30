using MageEditor.Common;
using MageEditor.Content;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Media3D;

namespace MageEditor.Editors
{
    // NOTE: purpose of this class is to enable viewing 3D geometry in WPF while we still don't have our own renderer in game engine.
    //       When we will have renderer, this class and the WPF viewer will become obsolete.
    class MeshRendererVertexData : ViewModelBase
    {
        // Brush is dependency object, this class is temporary, so we will allow it for now
        public Brush _specular = new SolidColorBrush((Color)ColorConverter.ConvertFromString("#ff111111")); // dark grey specular color
        public Brush Specular
        {
            get => _specular;
            set
            {
                if (_specular != value)
                {
                    _specular = value;
                    OnPropertyChanged(nameof(Specular));
                }
            }
        }

        // Brush is dependency object, this class is temporary, so we will allow it for now
        public Brush _diffuse = Brushes.White;
        public Brush Diffuse
        {
            get => _diffuse;
            set
            {
                if (_diffuse != value)
                {
                    _diffuse = value;
                    OnPropertyChanged(nameof(Diffuse));
                }
            }
        }


        public Point3DCollection Positions { get; } = new Point3DCollection();
        public Vector3DCollection Normals { get; } = new Vector3DCollection();
        public PointCollection UVs { get; } = new PointCollection();
        public Int32Collection Indices { get; } = new Int32Collection();
    }

    // NOTE: purpose of this class is to enable viewing 3D geometry in WPF while we still don't have our own renderer in game engine.
    //       When we will have renderer, this class and the WPF viewer will become obsolete.
    class MeshRenderer : ViewModelBase
    {
        public ObservableCollection<MeshRendererVertexData> Meshes { get; } = new ObservableCollection<MeshRendererVertexData>();

        private Vector3D _cameraDirection = new Vector3D(0, 0, -10); // opposite of position so that camera looks into the scene
        public Vector3D CameraDirection
        {
            get => _cameraDirection;
            set
            {
                if (_cameraDirection != value)
                {
                    _cameraDirection = value;
                    OnPropertyChanged(nameof(CameraDirection));
                }
            }
        }

        private Point3D _cameraPosition = new Point3D(0, 0, 10);
        public Point3D CameraPosition
        {
            get => _cameraPosition;
            set
            {
                if (_cameraPosition != value)
                {
                    _cameraPosition = value;
                    CameraDirection = new Vector3D(-value.X, -value.Y, -value.Z);
                    OnPropertyChanged(nameof(OffsetCameraPosition));
                    OnPropertyChanged(nameof(CameraPosition));
                }
            }
        }

        private Point3D _cameraTarget = new Point3D(0, 0, 0);
        public Point3D CameraTarget
        {
            get => _cameraTarget;
            set
            {
                if( _cameraTarget != value)
                {
                    _cameraTarget = value;
                    OnPropertyChanged(nameof(OffsetCameraPosition));
                    OnPropertyChanged(nameof(CameraTarget));
                }
            }
        }

        // so that we can move the camera with respect to the object that we're looking at
        public Point3D OffsetCameraPosition => 
            new Point3D(CameraPosition.X + CameraTarget.X, CameraPosition.Y + CameraTarget.Y, CameraPosition.Z + CameraTarget.Z);

        private Color _keyLightColor = (Color)ColorConverter.ConvertFromString("#ffaeaeae"); // grayish, not quite white color
        public Color KeyLightColor
        {
            get => _keyLightColor;
            set
            {
                if (_keyLightColor != value)
                {
                    _keyLightColor = value;
                    OnPropertyChanged(nameof(KeyLightColor));
                }
            }
        }

        private Color _skyLightColor = (Color)ColorConverter.ConvertFromString("#ff111b30");
        public Color SkyLightColor
        {
            get => _skyLightColor;
            set
            {
                if (_skyLightColor != value)
                {
                    _skyLightColor = value;
                    OnPropertyChanged(nameof(SkyLightColor));
                }
            }
        }

        private Color _groundLightColor = (Color)ColorConverter.ConvertFromString("#ff3f2f1e"); // warmer color coming from ground
        public Color GroundLightColor
        {
            get => _groundLightColor;
            set
            {
                if (_groundLightColor != value)
                {
                    _groundLightColor = value;
                    OnPropertyChanged(nameof(GroundLightColor));
                }
            }
        }

        private Color _ambientLightColor = (Color)ColorConverter.ConvertFromString("#ff3b3b3b"); // dim and neutral grey
        public Color AmbientLightColor
        {
            get => _ambientLightColor;
            set
            {
                if (_ambientLightColor != value)
                {
                    _ambientLightColor = value;
                    OnPropertyChanged(nameof(AmbientLightColor));
                }
            }
        }

        // each time we create new PrimitiveMesh, a new Renderer is also created, but we want to take over old camera position, direction
        public MeshRenderer(MeshLOD? lod, MeshRenderer? old)
        {
            Debug.Assert(lod?.Meshes.Any() == true);
            // calculate vertex size minus the position and normal vectors.
            // in mage::tools::packed_vertex there's some informations that we don't want to use here, e.g. u8 reversed[3], u16 tangent[2]
            // we skip over math::vec3 position (3 floats), 4 bytes for reserved[] and t_sign (sizeof int), 2*sizeof(short) for skipping normals
            // size of vertex is sizeof(mage::tools::packed_vertex::static_vertex) which is 32 bytes
            // therefore offset = 12 (unless size of vertex changes in future ofc)
            var offset = lod.Meshes[0].VertexSize - 3 * sizeof(float) -sizeof(int) - 2 * sizeof(short);
            // later on when we want to read UVs, we have to skip tangents as well

            // figure out BoundingBox, also where the avarage normal of the object is pointing to.
            // so that we can set up camera position and target it properly.
            double minX, minY, minZ; minX = minY = minZ = double.MaxValue;
            double maxX, maxY, maxZ; maxX = maxY = maxZ = double.MinValue;
            Vector3D avgNormal = new Vector3D();
            // in mage::tools::pack_vertices_static() we pack normals with pack_float<16>() with range [-1, 1]
            // so basically we got to do similar thing to mage::math::unpack_to_float<>() in C# side
            // so (i/intervals) * (1 - (-1)) + (-1) = (i/intervals) * 2 - 1
            var intervals = 2.0f / ((1 << 16) - 1); // to unpack packed normals


            foreach(var mesh in lod.Meshes )
            {
                var vertexData = new MeshRendererVertexData();
                // unpack all vertices -> data from m.packed_static_vertices.data() in mage::tools::pack_mesh_data() function
                using (var reader = new BinaryReader(new MemoryStream(mesh.Vertices)))
                {
                    for(int i = 0; i < mesh.VertexCount; ++i)
                    {
                        // read positions
                        var posX = reader.ReadSingle();
                        var posY = reader.ReadSingle();
                        var posZ = reader.ReadSingle();
                        // normal Z (sign) was included in 2nd bit of sign int, so we shift to the right, so we get last byte (u8)
                        // and bitwise AND just to be sure, we read these last byte
                        var signs = (reader.ReadUInt32() >> 24) & 0x000000ff;
                        vertexData.Positions.Add(new Point3D(posX, posY, posZ));

                        // adjust BoundingBox
                        minX = Math.Min(minX, posX); maxX = Math.Max(maxX, posX);
                        minY = Math.Min(minY, posY); maxY = Math.Max(maxY, posY);
                        minZ = Math.Min(minZ, posZ); maxZ = Math.Max(maxZ, posZ);

                        // read normals
                       
                        var normalX = reader.ReadUInt16() * intervals - 1.0f;
                        var normalY = reader.ReadUInt16() * intervals - 1.0f;
                        // pythagorean formula to calculate unit value
                        // sign for normalZ is packed in 2nd bit of signs value
                        var normalZ = Math.Sqrt(Math.Clamp(1f - (normalX * normalX + normalY * normalY), 0f, 1f)) * ((signs&0x2) - 1f);
                        var normal = new Vector3D(normalX, normalY, normalZ);
                        normal.Normalize();
                        vertexData.Normals.Add(normal);
                        avgNormal += normal;

                        // read UVs (skip tangent and joint data)
                        reader.BaseStream.Position += (offset - sizeof(float) * 2); // skip tangents (12 - 8) = 4
                        // read UV from vec2
                        var u  = reader.ReadSingle();
                        var v = reader.ReadSingle();
                        vertexData.UVs.Add(new Point(u, v));
                    }
                }

                // unpacking indices -> indices data copied to buffer at the end of mage::tools::pack_mesh_data() function
                using (var reader = new BinaryReader(new MemoryStream(mesh.Indices)))
                {
                    if(mesh.IndexSize == sizeof(short))
                        for(int i = 0; i < mesh.IndexCount; ++i) vertexData.Indices.Add(reader.ReadUInt16());
                    else
                        for(int i = 0; i < mesh.IndexCount; ++i) vertexData.Indices.Add(reader.ReadInt32());
                }
                // make object unmodifiable
                vertexData.Positions.Freeze();
                vertexData.Normals.Freeze();
                vertexData.UVs.Freeze();
                vertexData.Indices.Freeze();
                Meshes.Add(vertexData);
            }

            // set camera's target and position
            if(old != null)
            {
                CameraTarget = old.CameraTarget;
                CameraPosition = old.CameraPosition;
            }
            else
            {
                // compute bounding box dimensions
                var width = maxX - minX;
                var height = maxY - minY;
                var depth = maxZ - minZ;
                // note that the sphere's diamater is twice the size of the bounding box
                var radius = new Vector3D(height, width, depth).Length * 1.2;
                // we take that direction as a front of the object
                if (avgNormal.Length > 0.8)
                {
                    avgNormal.Normalize();
                    avgNormal *= radius;
                    CameraPosition = new Point3D(avgNormal.X, avgNormal.Y, avgNormal.Z);
                }
                else
                {
                    // set comera's position somewhere else outside object's bounding box
                    // at front of BoundingBox, halfway up, radius tells how far in Z direction camera is moved. 
                    CameraPosition = new Point3D(width, height * 0.5, radius);
                }

                // center of the objet that we're rendering
                CameraTarget = new Point3D(minX + width * 0.5, minY + height * 0.5, minZ + depth * 0.5);
            }
        }
    }

    class GeometryEditor : ViewModelBase, IAssetEditor
    {
        public Content.Asset Asset => Geometry;

        private Content.Geometry _geometry;
        public Content.Geometry Geometry
        {
            get => _geometry;
            set
            {
                if(_geometry != value)
                {
                    _geometry = value;
                    OnPropertyChanged(nameof(Geometry));
                }
            }
        }

        private MeshRenderer _meshRenderer;
        public MeshRenderer MeshRenderer
        {
            get => _meshRenderer;
            set
            {
                if(_meshRenderer != value)
                {
                    _meshRenderer = value;
                    OnPropertyChanged(nameof(MeshRenderer));
                }
            }
        }

        public void SetAsset(Content.Asset asset)
        {
            Debug.Assert(asset is Content.Geometry);
            if(asset is Content.Geometry geo)
            {
                Geometry = geo;
                // for now we take the biggest LOD
                MeshRenderer = new MeshRenderer(Geometry.GetLODGroup()?.LODs[0], MeshRenderer);
            }
        }

    }
}
