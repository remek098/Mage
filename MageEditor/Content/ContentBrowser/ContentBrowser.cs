using MageEditor.Common;
using MageEditor.GameProject;
using MageEditor.Utilities;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Windows;

namespace MageEditor.Content
{
    sealed class ContentInfo
    {
        public static int IconWidth => 90;
        public byte[]? Icon { get; }
        public byte[]? IconSmall { get; }
        public string FullPath { get; }
        public string FileName => Path.GetFileNameWithoutExtension(FullPath);
        public bool IsDirectory { get; }
        public DateTime DateModified { get; }
        public long? Size { get; } // nullable because directories don't have size.

        public ContentInfo(string fullPath, byte[]? icon = null, byte[]? smallIcon = null, DateTime? lastModified = null)
        {
            Debug.Assert(File.Exists(fullPath) || Directory.Exists(fullPath));
            var info = new FileInfo(fullPath);
            IsDirectory = ContentHelper.IsDirectory(fullPath);
            DateModified = lastModified ?? info.LastWriteTime;
            Size = IsDirectory ? (long?)null : info.Length;
            Icon = icon;
            IconSmall = smallIcon ?? icon; // smallIcon if it's not null, otherwise icon
            FullPath = fullPath;
        }
    }


    class ContentBrowser : ViewModelBase, IDisposable
    {
        // because we're calling GetFolderContent() method on both UI and non-UI threads.
        // by _refreshTimer with Refresh() method and in SelectedFolder property setter (on UI thread)
        public static readonly object _lock = new object();

        // if we don't get notified for 250 seconds, we can proceed to handle the event(s).
        private static readonly DelayedEventTimer _refreshTimer = new DelayedEventTimer(TimeSpan.FromMicroseconds(250));
        private static readonly FileSystemWatcher _contentWatcher = new FileSystemWatcher() {
            IncludeSubdirectories = true,
            Filter = "",
            NotifyFilter = NotifyFilters.CreationTime |
                           NotifyFilters.DirectoryName |
                           NotifyFilters.FileName |
                           NotifyFilters.LastWrite
        };


        public static string _cacheFilePath = string.Empty;
        private static readonly Dictionary<string, ContentInfo> _contentInfoCache = new Dictionary<string, ContentInfo>();


        public string ContentFolder { get; }
        
        private readonly ObservableCollection<ContentInfo> _folderContent = new ObservableCollection<ContentInfo>();
        public ReadOnlyObservableCollection<ContentInfo> FolderContent { get; }

        private string _selectedFolder;
        public string SelectedFolder
        {
            get => _selectedFolder;
            set
            {
                if( _selectedFolder != value ) {
                    _selectedFolder = value;
                    if(!string.IsNullOrEmpty(_selectedFolder)) {
                        GetFolderContent();
                    }
                    OnPropertyChanged(nameof(SelectedFolder));
                }   
            }
        }

        private async void GetFolderContent()
        {
            var folderContent = new List<ContentInfo>();
            await Task.Run(() =>
            {
                folderContent = GetFolderContent(SelectedFolder);
            });

            _folderContent.Clear();
            folderContent.ForEach(x => _folderContent.Add(x));
        }

        private static List<ContentInfo> GetFolderContent(string path)
        {
            Debug.Assert(!string.IsNullOrEmpty(path));
            var folderContent = new List<ContentInfo>();
            try {
                // get sub-folder
                foreach(var dir in Directory.GetDirectories(path)) {
                    folderContent.Add(new ContentInfo(dir));
                }

                // get files
                lock(_lock) {
                    foreach(var file in Directory.GetFiles(path, $"*{Asset.AssetFileExtension}")) {
                        var fileInfo = new FileInfo(file);

                        if(!_contentInfoCache.ContainsKey(file) ||
                            _contentInfoCache[file].DateModified.IsOlder(fileInfo.LastWriteTime)) {
                            // if the file is not already in the cache or if it has been updated,
                            // we got to reload it's information
                            var info = AssetRegistery.GetAssetInfo(file) ?? Asset.GetAssetInfo(file);
                            Debug.Assert(info != null);
                            _contentInfoCache[file] = new ContentInfo(file, info.Icon);
                        }

                        Debug.Assert(_contentInfoCache.ContainsKey(file));
                        folderContent.Add(_contentInfoCache[file]);
                    }
                } // _lock
            }
            catch (Exception ex) {
                Debug.WriteLine(ex.Message);
            }
            return folderContent;
        }

        private void Refresh(object? sender, DelayedEventTimerArgs e)
        {
            // happens when _refreshTimer triggers an event
            GetFolderContent();
        }

        private async void OnContentModified(object sender, FileSystemEventArgs e)
        {
            if (Path.GetDirectoryName(e.FullPath) != SelectedFolder) return;

            await Application.Current.Dispatcher.BeginInvoke(new Action(() =>
            {
                _refreshTimer.Trigger(e);
            }));
        }
        
        private static void SaveInfoCache(string file)
        {
            lock(_lock) {
                using var writer = new BinaryWriter(File.Open(file, FileMode.Create, FileAccess.Write));
                writer.Write(_contentInfoCache.Keys.Count); // numEntries
                foreach(var key in _contentInfoCache.Keys) {
                    var info = _contentInfoCache[key];

                    writer.Write(key); // assetFile
                    writer.Write(info.DateModified.ToBinary());
                    // NOTE: if there's no icon, we will write empty array into binary cache info file.
                    //       and we need to remember that empty array of byte type will be written to binary file,
                    //       which means during load we have to convert it back to null (if iconSize == 0)
                    writer.Write(info.Icon?.Length ?? 0); // icon size -> Int32
                    writer.Write(info.Icon ?? Array.Empty<byte>());
                }
            }
        }

        private static void LoadInfoCache(string file)
        {
            if (!File.Exists(file)) return;
            try {
                lock(_lock) {
                    using var reader = new BinaryReader(File.Open(file,FileMode.Open, FileAccess.Read));
                    var numEntries = reader.ReadInt32();
                    _contentInfoCache.Clear();

                    for(int i = 0; i < numEntries; ++i) {
                        var assetFile = reader.ReadString();
                        var date = DateTime.FromBinary(reader.ReadInt64());
                        var iconSize = reader.ReadInt32();
                        var icon = iconSize > 0 ? reader.ReadBytes(iconSize) : (byte[]?)null;

                        if(iconSize < 0) {
                            throw new InvalidDataException("invalid icon size in cache.");
                        }

                        if (File.Exists(assetFile)) {
                            _contentInfoCache[assetFile] = new ContentInfo(assetFile, icon, null, date);
                        }
                    }
                }
            }
            catch (Exception ex) {
                Debug.WriteLine(ex.Message);
                Logger.Log(MessageType.Warning, "Failed to read Content Browser's cache file.");
                _contentInfoCache.Clear();
            }
        }


        public void Dispose()
        {
            ((IDisposable)_contentWatcher).Dispose();
            if(!string.IsNullOrEmpty(_cacheFilePath)) {
                SaveInfoCache(_cacheFilePath);
                _cacheFilePath = string.Empty;
            }
        }

        public ContentBrowser(Project project)
        {
            Debug.Assert(project != null);
            var contentFolder = project.ContentPath;
            Debug.Assert(!string.IsNullOrEmpty(contentFolder.Trim()));
            contentFolder = Path.TrimEndingDirectorySeparator(contentFolder);
            ContentFolder = contentFolder;
            SelectedFolder = contentFolder;
            FolderContent = new ReadOnlyObservableCollection<ContentInfo>(_folderContent);

            if(string.IsNullOrEmpty(_cacheFilePath)) {
                _cacheFilePath = $@"{project.Path}.Mage\ContentInfoCache.bin";
                LoadInfoCache(_cacheFilePath);
            }

            _contentWatcher.Path = contentFolder;
            _contentWatcher.Changed += OnContentModified;
            _contentWatcher.Created += OnContentModified;
            _contentWatcher.Deleted += OnContentModified;
            _contentWatcher.Renamed += OnContentModified;
            _contentWatcher.EnableRaisingEvents = true;

            _refreshTimer.Triggered += Refresh;
        }

    }
}
