using MageEditor.Utilities;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Text;

namespace MageEditor.Content
{
    static class ContentInfoCache
    {
        private static readonly object _lock = new object();
        private static readonly Dictionary<string, ContentInfo> _contentInfoCache = new Dictionary<string, ContentInfo>();
        private static bool _isDirty;
        public static string _cacheFilePath = string.Empty;

        public static ContentInfo Add(string file)
        {
            lock (_lock) {
                var fileInfo = new FileInfo(file);
                Debug.Assert(!fileInfo.IsDirectory());
                
                // we cache content info only if it's not already in the cache or is newer than a cache entry.
                if (!_contentInfoCache.ContainsKey(file) ||
                     _contentInfoCache[file].DateModified.IsOlder(fileInfo.LastWriteTime)) {
                    // if the file is not already in the cache or if it has been updated,
                    // we got to reload it's information
                    var info = AssetRegistery.GetAssetInfo(file) ?? Asset.GetAssetInfo(file);
                    Debug.Assert(info != null);
                    _contentInfoCache[file] = new ContentInfo(file, info.Icon);

                    _isDirty = true; // gotta do that mate. Cache needs overwrite ASAP
                }

                Debug.Assert(_contentInfoCache.ContainsKey(file));
                return _contentInfoCache[file];
            }
        }

        public static void Reset(string projectPath)
        {
            lock(_lock) {
                if(!string.IsNullOrEmpty(_cacheFilePath) && _isDirty) {
                    SaveInfoCache();
                    _cacheFilePath = string.Empty;
                    _contentInfoCache.Clear();
                    _isDirty = false;
                }

                if (!string.IsNullOrEmpty(projectPath)) {
                    Debug.Assert(Directory.Exists(projectPath));
                    _cacheFilePath = $@"{projectPath}.Mage\ContentInfoCache.bin";
                    LoadInfoCache();
                }
            }
        }

        public static void Save() => Reset(string.Empty);

        private static void SaveInfoCache()
        {
            try {
                using var writer = new BinaryWriter(File.Open(_cacheFilePath, FileMode.Create, FileAccess.Write));
                writer.Write(_contentInfoCache.Keys.Count); // numEntries
                foreach (var key in _contentInfoCache.Keys) {
                    var info = _contentInfoCache[key];

                    writer.Write(key); // assetFile
                    writer.Write(info.DateModified.ToBinary());
                    // NOTE: if there's no icon, we will write empty array into binary cache info file.
                    //       and we need to remember that empty array of byte type will be written to binary file,
                    //       which means during load we have to convert it back to null (if iconSize == 0)
                    writer.Write(info.Icon?.Length ?? 0); // icon size -> Int32
                    writer.Write(info.Icon ?? Array.Empty<byte>());
                }

                _isDirty = false;
            }
            catch (Exception ex) {
                Debug.WriteLine(ex.Message);
                Logger.Log(MessageType.Warning, "Failed to save Content Browser's cache file.");
            }
        }

        private static void LoadInfoCache()
        {
            if (!File.Exists(_cacheFilePath)) return;
            try {
                using var reader = new BinaryReader(File.Open(_cacheFilePath, FileMode.Open, FileAccess.Read));
                var numEntries = reader.ReadInt32();
                _contentInfoCache.Clear();

                for (int i = 0; i < numEntries; ++i) {
                    var assetFile = reader.ReadString();
                    var date = DateTime.FromBinary(reader.ReadInt64());
                    var iconSize = reader.ReadInt32();
                    var icon = iconSize > 0 ? reader.ReadBytes(iconSize) : (byte[]?)null;

                    if (iconSize < 0) {
                        throw new InvalidDataException("invalid icon size in cache.");
                    }

                    if (File.Exists(assetFile)) {
                        _contentInfoCache[assetFile] = new ContentInfo(assetFile, icon, null, date);
                    }
                }
            }
            catch (Exception ex) {
                Debug.WriteLine(ex.Message);
                Logger.Log(MessageType.Warning, "Failed to read Content Browser's cache file.");
                _contentInfoCache.Clear();
            }
        }

    }
}
