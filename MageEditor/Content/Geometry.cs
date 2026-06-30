using MageEditor.Common;
using MageEditor.Utilities;
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
using System.Windows.Media.Imaging;

namespace MageEditor.Content
{
    enum PrimitiveMeshType
    {
        Plane,
        Cube,
        UvSphere,
        IcoSphere,
        Cylinder,
        Capsule
    }

    class Mesh : ViewModelBase
    {
        private int _vertexSize;
        public int VertexSize
        {
            get => _vertexSize;
            set
            {
                if (_vertexSize != value)
                {
                    _vertexSize = value;
                    OnPropertyChanged(nameof(VertexSize));
                }
            }
        }

        private int _vertexCount;
        public int VertexCount
        {
            get => _vertexCount;
            set
            {
                if (_vertexCount != value)
                {
                    _vertexCount = value;
                    OnPropertyChanged(nameof(VertexCount));
                }
            }
        }


        private int _indexSize;
        public int IndexSize
        {
            get => _indexSize;
            set
            {
                if (_indexSize != value)
                {
                    _indexSize = value;
                    OnPropertyChanged(nameof(IndexSize));
                }
            }
        }

        private int _indexCount;
        public int IndexCount
        {
            get => _indexCount;
            set
            {
                if (_indexCount != value)
                {
                    _indexCount = value;
                    OnPropertyChanged(nameof(IndexCount));
                }
            }
        }

        public byte[] Vertices { get; set; }
        public byte[] Indices { get; set; }
    }

    class MeshLOD : ViewModelBase
    {
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

        private float _lodTreshold;
        public float LODTreshold
        {
            get => _lodTreshold;
            set
            {
                if (_lodTreshold != value)
                {
                    _lodTreshold = value;
                    OnPropertyChanged(nameof(LODTreshold));
                }
            }
        }

        public ObservableCollection<Mesh> Meshes { get; } = new ObservableCollection<Mesh>();
    }

    class LODGroup : ViewModelBase
    {
        private string _name;
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

        
        public ObservableCollection<MeshLOD> LODs { get; } = new ObservableCollection<MeshLOD>();
    }

    class GeometryImportSettings : ViewModelBase
    {
        // as it is in ContentToolsAPIStructs.GeometryImportSettings
        //public float SmoothingAngle = 178f;
        //public byte CalculateNormals = 0;
        //public byte CalculateTangents = 1;
        //public byte ReverseHandedness = 0;
        //public byte ImportEmbededTextures = 1;
        //public byte ImportAnimations = 1;

        private bool _calculateNormals;
        public bool CalculateNormals
        {
            get => _calculateNormals;
            set
            {
                if (_calculateNormals != value)
                {
                    _calculateNormals = value;
                    OnPropertyChanged(nameof(CalculateNormals));
                }
            }
        }

        private bool _calculateTangents;
        public bool CalculateTangents
        {
            get => _calculateTangents;
            set
            {
                if( _calculateTangents != value)
                {
                    _calculateTangents = value;
                    OnPropertyChanged(nameof(CalculateTangents));
                }
            }
        }

        private float _smoothingAngle;
        public float SmoothingAngle
        {
            get => _smoothingAngle;
            set
            {
                if( _smoothingAngle != value)
                {
                    _smoothingAngle = value;
                    OnPropertyChanged(nameof(SmoothingAngle));
                }
            }
        }

        private bool _reverseHandedness;
        public bool ReverseHandedness
        {
            get => _reverseHandedness;
            set
            {
                if(_reverseHandedness != value)
                {
                    _reverseHandedness = value;
                    OnPropertyChanged(nameof(ReverseHandedness));
                }
            }
        }

        private bool _importEmbededTextures;
        public bool ImportEmbededTextures
        {
            get => _importEmbededTextures;
            set
            {
                if(_importEmbededTextures != value)
                {
                    _importEmbededTextures = value;
                    OnPropertyChanged(nameof(ImportEmbededTextures));
                }
            }
        }

        private bool _importAnimations;
        public bool ImportAnimations
        {
            get => _importAnimations;
            set
            {
                if(_importAnimations != value)
                {
                    _importAnimations = value;
                    OnPropertyChanged(nameof(ImportAnimations));
                }
            }
        }

        public GeometryImportSettings()
        {
            CalculateNormals = false;
            CalculateTangents = false;
            SmoothingAngle = 178f;
            ReverseHandedness = false;
            ImportEmbededTextures = true;
            ImportAnimations = true;
        }

        public void ToBinary(BinaryWriter writer)
        {
            writer.Write(CalculateNormals);
            writer.Write(CalculateTangents);
            writer.Write(SmoothingAngle);
            writer.Write(ReverseHandedness);
            writer.Write(ImportEmbededTextures);
            writer.Write(ImportAnimations);
        }
    }

    class Geometry : Asset
    {
        private readonly List<LODGroup> _lodGroups = new List<LODGroup>();
        public GeometryImportSettings ImportSettings { get; } = new GeometryImportSettings();

        public LODGroup? GetLODGroup(int lodGroup = 0)
        {
            Debug.Assert(lodGroup >= 0 && lodGroup < _lodGroups.Count);
            return _lodGroups.Any() ? _lodGroups[lodGroup] : null;
        }

        public void FromRawData(byte[] data)
        {
            // mage::tools::scene has kinda tree structure -> name + vector<lod_group>
            // we need to work with output data of engine's mesh struct here
            Debug.Assert(data?.Length > 0);
            _lodGroups.Clear(); // throw away any data we have, so that we can get new data in

            using var reader = new BinaryReader(new MemoryStream(data));
            // we will basically read data according to how it was packed/saved in mage::tools::pack_data()
            
            // skip scene name string -> since we don't use it for now
            var s = reader.ReadInt32();
            reader.BaseStream.Position += s; // skipping scene name
            // get number of LODs
            var numLODGroups = reader.ReadInt32();
            Debug.Assert(numLODGroups > 0);

            for(int i = 0; i < numLODGroups; ++i)
            {
                // get LOD group's name
                s = reader.ReadInt32();
                string lodGroupName;
                if(s > 0)
                {
                    var nameBytes = reader.ReadBytes(s);
                    lodGroupName = Encoding.UTF8.GetString(nameBytes);
                }
                else
                {
                    lodGroupName = $"lod_{ContentHelper.GetRandomString()}";
                }

                // get number of meshes in the LOD group
                var numMeshes = reader.ReadInt32();
                Debug.Assert(numMeshes > 0);
                var lods = ReadMeshLODs(numMeshes, reader);

                var lodGroup = new LODGroup() { Name = lodGroupName };
                lods.ForEach(l => lodGroup.LODs.Add(l));
                _lodGroups.Add(lodGroup);
            }
        }

        private static List<MeshLOD> ReadMeshLODs(int numMeshes, BinaryReader reader)
        {
            var lodIDs = new List<int>(); // we have to look at lod_id's of mage::tools::mesh's
                                          // and figure out to which LOD group the mesh goes
            var lodList = new List<MeshLOD>();
            for(int i = 0; i < numMeshes; ++i)
            {
                ReadMeshes(reader, lodIDs, lodList);
            }

            return lodList;
        }

        private static void ReadMeshes(BinaryReader reader, List<int> lodIDs, List<MeshLOD> lodList)
        {
            // for reference if smth is unclear check mage::tools::pack_mesh_data()
            // get meshe's name
            var s = reader.ReadInt32();
            string meshName; // read it if we have a right one, otherwise generate random string
            if (s > 0)
            {
                var nameBytes = reader.ReadBytes(s);
                meshName = Encoding.UTF8.GetString(nameBytes);
            }
            else
            {
                meshName = $"mesh_{ContentHelper.GetRandomString()}";
            }

            var mesh = new Mesh();
            var lodID = reader.ReadInt32();
            mesh.VertexSize = reader.ReadInt32();
            mesh.VertexCount = reader.ReadInt32();
            mesh.IndexSize = reader.ReadInt32();
            mesh.IndexCount = reader.ReadInt32();
            var lodTreshold = reader.ReadSingle(); // treshold is packed as f32

            // sizes of vertex and index buffers
            var vBufferSize = mesh.VertexSize * mesh.VertexCount;
            var iBufferSize = mesh.IndexSize * mesh.IndexCount;

            mesh.Vertices = reader.ReadBytes(vBufferSize);
            mesh.Indices = reader.ReadBytes(iBufferSize);

            MeshLOD lod;
            if (ID.IsValid(lodID) && lodIDs.Contains(lodID))
            {
                // if we already have valid id that also exists in our lodList, we just output our mesh in this lod
                lod = lodList[lodIDs.IndexOf(lodID)];
                Debug.Assert(lod != null);
            }
            else
            {
                // otherwise we need to create a new MeshLOD
                lodIDs.Add(lodID); // add it so we can look it up later
                lod = new MeshLOD() { Name = meshName, LODTreshold = lodTreshold };
                lodList.Add(lod);
            }
            lod.Meshes.Add(mesh);
        }

        public override IEnumerable<string> Save(string file)
        {
            // LODGroup represents a collection of Mesh assets that we have under our Geometry asset class.
            Debug.Assert(_lodGroups.Any());
            var savedFiles = new List<string>();
            if(!_lodGroups.Any()) return savedFiles; // return empty list if we don't have any Meshes to save

            // path where the file resides
            var path = Path.GetDirectoryName(file) + Path.DirectorySeparatorChar;
            var fileName = Path.GetFileNameWithoutExtension(file);

            try {
                // save meshes/assets for each LODGroup 1 by 1
                foreach(var lod_group in _lodGroups) {
                    Debug.Assert(lod_group.LODs.Any());
                    // use the name of the most detailed LOD for file name
                    var meshFileName = ContentHelper.SanitizeFileName(path + fileName + "_" + lod_group.LODs[0].Name + AssetFileExtension);
                    // NOTE: we have to make a diffrent id for each newly created asset file.
                    Guid = Guid.NewGuid();
                    byte[]? data = null;
                    using(var writer = new BinaryWriter(new MemoryStream())) {
                        writer.Write(lod_group.Name); // name of object
                        writer.Write(lod_group.LODs.Count); // number of submeshes that are contained in this object
                        var hashes = new List<byte>();
                        foreach(var lod in lod_group.LODs) {
                            LODToBinary(lod, writer, out var hash);
                            if(hash != null) hashes.AddRange(hash);
                        }

                        Hash = ContentHelper.ComputeHash(hashes.ToArray());
                        data = (writer.BaseStream as MemoryStream)?.ToArray();
                        Icon = GenerateIcon(lod_group.LODs[0]);
                    }

                    Debug.Assert(data?.Length > 0); // check that we actually wrote something KEKW
                    using(var writer = new BinaryWriter(File.Open(meshFileName, FileMode.Create, FileAccess.Write))) {
                        WriteAssetFileHeader(writer);
                        // save import settings with the geometry asset as well. -> so that we know for future
                        ImportSettings.ToBinary(writer);
                        writer.Write(data.Length);
                        writer.Write(data);
                    }

                    savedFiles.Add(meshFileName);
                }
            }
            catch (Exception ex) {
                Debug.WriteLine(ex.Message);
                Logger.Log(MessageType.Error, $"Failed to save geometry to {file}");
            }
            return savedFiles;
        }

        

        private void LODToBinary(MeshLOD lod, BinaryWriter writer, out byte[]? hash)
        {
            writer.Write(lod.Name);
            writer.Write(lod.LODTreshold);
            writer.Write(lod.Meshes.Count);

            // we want to calculate a hash for mesh data only, not the names for LODs and other things
            var meshDataBeginWriterPosition = writer.BaseStream.Position;

            // save anything we need to know about the mesh -> we calculate hash for this data only,
            // because if anything is diffrent, than it means that our mesh is diffrent
            // we can have same mesh with diffrent names, but that shouldn't matter, since it's still a duplicate.
            foreach(var mesh in lod.Meshes) {
                writer.Write(mesh.VertexSize);
                writer.Write(mesh.VertexCount);
                writer.Write(mesh.IndexSize);
                writer.Write(mesh.IndexCount);
                writer.Write(mesh.Vertices);
                writer.Write(mesh.Indices);
            }
            var meshDataSize = writer.BaseStream.Position - meshDataBeginWriterPosition;
            Debug.Assert(meshDataSize > 0); // number of bytes we've written for meshes

            var buffer = (writer.BaseStream as MemoryStream)?.ToArray();
            hash = ContentHelper.ComputeHash(buffer, (int)meshDataBeginWriterPosition, (int) meshDataSize);
        }

        private byte[] GenerateIcon(MeshLOD lod)
        {
            var width = 90 * 4; // we will render 4x wider image and then downsample it (softer edges for rendered object)
            BitmapSource? bmp = null;

            // this makes sure that it's executed on UI thread
            // NOTE: it's not good practice to use WPF control (view) in the ViewModel.
            //       But we have to make exception here, for as long as we don't have graphics renderer that we can use 
            //       for screenshots.
            Application.Current.Dispatcher.Invoke(() =>
            {
                bmp = Editors.GeometryView.RenderToBitmap(new Editors.MeshRenderer(lod, null), width, width);
                bmp = new TransformedBitmap(bmp, new ScaleTransform(0.25, 0.25, 0.5, 0.5));
            });

            using var memoryStream = new MemoryStream();
            memoryStream.SetLength(0);
            // encode a bitmap we just rendered into .png format
            var encoder = new PngBitmapEncoder();
            encoder.Frames.Add(BitmapFrame.Create(bmp));
            encoder.Save(memoryStream);

            return memoryStream.ToArray(); // return a stream that contains out .png image
        }

        public Geometry() : base(AssetType.Mesh)
        {
        }
    }

}
