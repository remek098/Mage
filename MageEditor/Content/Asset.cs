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

    // can be used to bind properties to our views
    abstract class Asset : ViewModelBase
    {
        public static string AssetFileExtension => ".asset";

        public AssetType Type { get; private set; }
        public byte[] Icon { get; protected set; }
        public string SourcePath { get; protected set; }
        public Guid Guid { get; protected set; } = Guid.NewGuid();
        public DateTime ImportDate { get; protected set; }
        /// <summary>
        /// So that we know if we for some reason don't have duplicate assets.
        /// 
        /// <para>So that we can ask if we don't want to get rid of one of them.</para>
        /// </summary>
        public byte[]? Hash { get; protected set; }

        /// <summary>
        /// Saves file(s) to a specified location.
        /// </summary>
        /// <param name="file_location"></param>
        /// <returns></returns>
        public abstract IEnumerable<string> Save(string file);

        protected void WriteAssetFileHeader(BinaryWriter writer)
        {
            var id = Guid.ToByteArray();
            // NOTE: for now since we don't have any asset and we can test stuff
            var importDate = DateTime.Now.ToBinary();

            writer.BaseStream.Position = 0; // set the bad boy to the start of the file

            writer.Write((int)Type);
            writer.Write(id.Length); // so that we can read the array back later
            writer.Write(id);
            writer.Write(importDate);
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

        public Asset(AssetType type)
        {
            Debug.Assert(type != AssetType.Unknown);
            Type = type;
        }
    }
}
