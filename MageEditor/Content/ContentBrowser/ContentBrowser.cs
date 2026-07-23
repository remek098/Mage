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
        private static readonly DelayedEventTimer _refreshTimer = new DelayedEventTimer(TimeSpan.FromMicroseconds(250));


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
                        _ = GetFolderContent();
                    }
                    OnPropertyChanged(nameof(SelectedFolder));
                }   
            }
        }
        private void OnContentModified(object? sender, ContentModifiedEventArgs e)
        {
            if (Path.GetDirectoryName(e.FullPath) != SelectedFolder) return;
            _refreshTimer.Trigger();
        }

        private void Refresh(object? sender, DelayedEventTimerArgs e)
        {
            // happens when _refreshTimer triggers an event
            _ = GetFolderContent();
        }

        private async Task GetFolderContent()
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

                // gather information about files and directories in Content.
                foreach(var file in Directory.GetFiles(path, $"*{Asset.AssetFileExtension}")) {
                    var fileInfo = new FileInfo(file);
                    folderContent.Add(ContentInfoCache.Add(file));
                }
            }
            catch (Exception ex) {
                Debug.WriteLine(ex.Message);
            }
            return folderContent;
        }


        public void Dispose()
        {
            ContentWatcher.ContentModified -= OnContentModified;
            ContentInfoCache.Save(); // only writes to cache file, if it contains new information.
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

            ContentWatcher.ContentModified += OnContentModified;
            _refreshTimer.Triggered += Refresh;
        }

    }
}
