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

    class Geometry : Asset
    {
        private readonly List<LODGroup> _lodGroups = new List<LODGroup>();

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

        public Geometry() : base(AssetType.Mesh)
        {
        }
    }

}
