using MageEditor.Common;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MageEditor.Content
{
    enum AssetType
    {
        Unknown,
        Animation,
        Audio,
        Material,
        Mesh,
        Skeleton,
        Texture,
    }

    sealed class AssetInfo
    {
        public AssetType Type { get; set;  }
        public byte[] Icon { get; set;  }
        public string FullPath { get; set; }
        public string FileName => Path.GetFileName(FullPath);
        public string SourcePath { get; set; }
        public DateTime RegisterTime { get; set; }
        public DateTime ImportDate { get; set;  }
        public Guid Guid { get; set; }
        public byte[] Hash { get; set; } // used to quickly compare if the assets are the same
    }

    // can be used to bind properties to our views
    abstract class Asset : ViewModelBase
    {
        public static string AssetFileExtension => ".asset";

        public AssetType Type { get; private set; }
        public byte[] Icon { get; protected set; }
        public string SourcePath { get; protected set; }

        private string _fullPath;
        public string FullPath
        {
            get => _fullPath;
            set
            {
                if(_fullPath != value) {
                    _fullPath = value;
                    OnPropertyChanged(nameof(FullPath));
                    OnPropertyChanged(nameof(FileName));
                }
            }
        }

        public string FileName => Path.GetFileName(FullPath);


        public Guid Guid { get; protected set; } = Guid.NewGuid();
        public DateTime ImportDate { get; protected set; }
        /// <summary>
        /// So that we know if we for some reason don't have duplicate assets.
        /// 
        /// <para>So that we can ask if we don't want to get rid of one of them.</para>
        /// </summary>
        public byte[]? Hash { get; protected set; }

        public abstract void Import(string file);

        public abstract void Load(string file);

        /// <summary>
        /// Saves file(s) to a specified location.
        /// </summary>
        /// <param name="file_location"></param>
        /// <returns></returns>
        public abstract IEnumerable<string> Save(string file);
        public abstract byte[] PackForEngine();

        public static AssetInfo? TryGetAssetInfo(string file) =>
            File.Exists(file) && Path.GetExtension(file) == AssetFileExtension ? AssetRegistery.GetAssetInfo(file) ?? GetAssetInfo(file) : null;
        
        private static AssetInfo GetAssetInfo(BinaryReader reader)
        {
            // does similar thing to what WriteAssetFileHeader does, but will read instead of writing.
            reader.BaseStream.Position = 0;
            var info = new AssetInfo();
            info.Type = (AssetType)reader.ReadInt32();
            var idSize = reader.ReadInt32(); // id.Length
            info.Guid = new Guid(reader.ReadBytes(idSize)); // reads id
            info.ImportDate = DateTime.FromBinary(reader.ReadInt64());
            var hashSize = reader.ReadInt32();
            if(hashSize > 0) {
                info.Hash = reader.ReadBytes(hashSize);
            }
            info.SourcePath = reader.ReadString();
            var iconSize = reader.ReadInt32();
            info.Icon = reader.ReadBytes(iconSize);

            return info;
        }

        public static AssetInfo? GetAssetInfo(string file)
        {
            Debug.Assert(File.Exists(file) && Path.GetExtension(file) == AssetFileExtension);
            try {
                using var reader = new BinaryReader(File.Open(file, FileMode.Open, FileAccess.Read));
                var info = GetAssetInfo(reader);
                info.FullPath = file;
                return info;
            }
            catch (Exception ex) {
                Debug.WriteLine(ex.Message);
            }
            return null;
        }


        protected void WriteAssetFileHeader(BinaryWriter writer)
        {
            var id = Guid.ToByteArray();
            // NOTE: for now since we don't have any asset and we can test stuff
            var importDate = DateTime.Now.ToBinary();

            writer.BaseStream.Position = 0; // set the bad boy to the start of the file

            writer.Write((int)Type);
            writer.Write(id.Length); // so that we can read the array back later
            writer.Write(id);
            writer.Write(importDate); // that's 8 bytes
            // asset hash is optional
            if(Hash?.Length > 0) {
                writer.Write(Hash.Length);
                writer.Write(Hash);
            }
            else {
                writer.Write(0);
            }

            writer.Write(SourcePath ?? "");
            writer.Write(Icon.Length);
            writer.Write(Icon);
        }

        protected void ReadAssetFileHeader(BinaryReader reader)
        {
            var info = GetAssetInfo(reader);
            Debug.Assert(Type == info.Type);
            Guid = info.Guid;
            ImportDate = info.ImportDate;
            Hash = info.Hash;
            SourcePath = info.SourcePath;
            Icon = info.Icon;

        }

        public Asset(AssetType type)
        {
            Debug.Assert(type != AssetType.Unknown);
            Type = type;
        }
    }
}
