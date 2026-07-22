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
    /// <summary>
    /// Provide access and lookup functionality to anything that uses a reference to the asset
    /// </summary>
    static class AssetRegistery
    {
        // if we don't get notified for 250 seconds, we can proceed to handle the event(s).
        private static readonly DelayedEventTimer _refreshTimer = new DelayedEventTimer(TimeSpan.FromMicroseconds(250));
        private static readonly Dictionary<string, AssetInfo> _assetDictionary = new Dictionary<string, AssetInfo>();
        private static readonly ObservableCollection<AssetInfo> _assets = new ObservableCollection<AssetInfo>();
        private static readonly FileSystemWatcher _contentWatcher = new FileSystemWatcher() {
            IncludeSubdirectories = true,
            Filter = "",
            NotifyFilter = NotifyFilters.CreationTime |
                           NotifyFilters.DirectoryName |
                           NotifyFilters.FileName |
                           NotifyFilters.LastWrite
        };

        public static ReadOnlyObservableCollection<AssetInfo> Assets { get; } = new ReadOnlyObservableCollection<AssetInfo>(_assets);
        private static void RegisterAllAssets(string path)
        {
            Debug.Assert(Directory.Exists(path));
            foreach(var entry in Directory.GetFileSystemEntries(path)) {
                if(ContentHelper.IsDirectory(entry)) {
                    RegisterAllAssets(entry);
                }
                else {
                    RegisterAsset(entry);
                }
            }
        }

        private static void RegisterAsset(string file)
        {
            Debug.Assert(File.Exists(file));
            try {
                var fileInfo = new FileInfo(file);
                if(!_assetDictionary.ContainsKey(file) || 
                    _assetDictionary[file].RegisterTime.IsOlder(fileInfo.LastWriteTime)) {
                    var info = Asset.GetAssetInfo(file);
                    Debug.Assert(info != null);
                    info.RegisterTime = DateTime.Now;
                    _assetDictionary[file] = info;
                
                    Debug.Assert(_assetDictionary.ContainsKey(file));
                    _assets.Add(_assetDictionary[file]);
                }

            }
            catch (Exception ex) {
                Debug.WriteLine(ex.Message);
            }
        }

        public static void Clear()
        {
            _contentWatcher.EnableRaisingEvents = false;
            _assetDictionary.Clear();
            _assets.Clear();
        }

        public static void Reset(string contentFolder)
        {
            Clear();
            Debug.Assert(Directory.Exists(contentFolder));
            RegisterAllAssets(contentFolder);
            _contentWatcher.Path = contentFolder;
            _contentWatcher.EnableRaisingEvents = true;
        }

        private static void UnregisterAsset(string file)
        {
            if (_assetDictionary.ContainsKey(file)) {
                _assets.Remove(_assetDictionary[file]);
                _assetDictionary.Remove(file);
            }
        }


        private static async void OnContentModified(object sender, FileSystemEventArgs e)
        {
            if (Path.GetExtension(e.FullPath) != Asset.AssetFileExtension) return;
            // NOTE: due to the fact that this function can be called by other threads, we need to put 
            // anything that can modify our Collection on the UI (aka application's thread)
            await Application.Current.Dispatcher.BeginInvoke(new Action(() =>
            {
                _refreshTimer.Trigger(e);
            }));
        }
        private static void Refresh(object? sender, DelayedEventTimerArgs e)
        {
            foreach(var item in e.Data) {
                if (!(item is FileSystemEventArgs eventArgs)) continue;

                if(eventArgs.ChangeType == WatcherChangeTypes.Deleted) {
                    UnregisterAsset(eventArgs.FullPath);
                }
                else {
                    RegisterAsset(eventArgs.FullPath);
                    if(eventArgs.ChangeType == WatcherChangeTypes.Renamed) {
                        _assetDictionary.Keys.Where(key => !File.Exists(key)).ToList().ForEach(file => UnregisterAsset(file));
                    }
                }
            }
        }

        public static AssetInfo? GetAssetInfo(string file) => _assetDictionary.ContainsKey(file) ? _assetDictionary[file] : null;
        public static AssetInfo? GetAssetInfo(Guid guid) => _assets.FirstOrDefault(x => x.Guid == guid);

        static AssetRegistery()
        {
            _contentWatcher.Changed += OnContentModified;
            _contentWatcher.Created += OnContentModified;
            _contentWatcher.Deleted += OnContentModified;
            _contentWatcher.Renamed += OnContentModified;

            _refreshTimer.Triggered += Refresh;
        }

    }
}
