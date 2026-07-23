using MageEditor.Utilities;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Windows;

namespace MageEditor.Content
{
    public class ContentModifiedEventArgs : EventArgs
    {
        public string FullPath { get; }

        public ContentModifiedEventArgs(string path)
        {
            FullPath = path;
        }
    }


    static class ContentWatcher
    {
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

        // file watcher is only enabled when this counter is 0.
        private static int _fileWatcherEnableCounter = 0;
        public static event EventHandler<ContentModifiedEventArgs> ContentModified;

        public static void EnableFileWatcher(bool isEnabled)
        {
            // NOTE: editor will import files asynchronously, therefore there's no way we can determine when the entire operation of
            // importing a few files is done with booleans.
            if(_fileWatcherEnableCounter > 0 && isEnabled) {
                --_fileWatcherEnableCounter;
            } 
            else if(!isEnabled) {
                ++_fileWatcherEnableCounter;
            }
        }

        public static void Reset(string contentFolder, string projectPath)
        {
            _contentWatcher.EnableRaisingEvents = false;

            ContentInfoCache.Reset(projectPath);

            if(!string.IsNullOrEmpty(contentFolder)) {
                Debug.Assert(Directory.Exists(contentFolder));
                _contentWatcher.Path = contentFolder;
                _contentWatcher.EnableRaisingEvents = true;
                AssetRegistery.Reset(contentFolder); // reset that, so that it can register assets for new/other project.
            }
        }

        private static async void OnContentModified(object sender, FileSystemEventArgs e)
        {
            await Application.Current.Dispatcher.BeginInvoke(new Action(() => _refreshTimer.Trigger(e)));
        }
        private static void Refresh(object? sender, DelayedEventTimerArgs e)
        {
            if(_fileWatcherEnableCounter > 0) {
                e.RepeatEvent = true;
                return;
            }

            e.Data.Cast<FileSystemEventArgs>()
                .GroupBy(x => x.FullPath)
                .Select(x => x.First())
                .ToList()
                .ForEach(x => ContentModified?.Invoke(null, new ContentModifiedEventArgs(x.FullPath)));
        }


        static ContentWatcher()
        {
            _contentWatcher.Changed += OnContentModified;
            _contentWatcher.Created += OnContentModified;
            _contentWatcher.Deleted += OnContentModified;
            _contentWatcher.Renamed += OnContentModified;

            _refreshTimer.Triggered += Refresh;
        }

    }

}
