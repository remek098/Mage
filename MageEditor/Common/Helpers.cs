using MageEditor.Content;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media;

namespace MageEditor
{
    static class VisualExtensions
    {
        public static T? FindVisualParent<T>(this DependencyObject dependencyObject) where T : DependencyObject
        {
            // we only want to walk through Visual Tree
            if (!(dependencyObject is Visual)) return null;

            var parent = VisualTreeHelper.GetParent(dependencyObject);
            while(parent != null)
            {
                if(parent is T type)
                {
                    return type;
                }
                parent = VisualTreeHelper.GetParent(parent);
            }
            return null;
        }
    }

    static class ContentHelper
    {
        public static string GetRandomString(int length = 8)
        {
            if(length <= 0) length = 8;
            var n = length / 11;
            var sb = new StringBuilder();

            // this loop generates multiple of 11 characters
            for( int i = 0; i <= n; ++i)
            {
                // GetRandomFileName() generates strings like "fs483abc.xyz" (always 11 characters after removing dot)
                sb.Append(Path.GetRandomFileName().Replace(".", ""));
            }
            // so e.g. if we requested length of 12 characters, sb would create a 22 character long string, we just take 
            // first 12
            return sb.ToString(0, length);
        }

        public static bool IsDirectory(string path)
        {
            try {
                return File.GetAttributes(path).HasFlag(FileAttributes.Directory);
            }
            catch (Exception ex) { Debug.WriteLine(ex.Message); }
            return false;
        }

        public static bool IsDirectory(this FileInfo info) => info.Attributes.HasFlag(FileAttributes.Directory);

        public static bool IsOlder(this DateTime date, DateTime other) => date < other;

        public static string SanitizeFileName(string name)
        {
            var path = new StringBuilder(name.Substring(0, name.LastIndexOf(Path.DirectorySeparatorChar) + 1));

            // e.g. string name = @"C:\Users\Alice\document.txt"; -> file = StringBuilder("document.txt") -> .. operator just retuns range
            var file = new StringBuilder(name[(name.LastIndexOf(Path.DirectorySeparatorChar) + 1)..]);
            // replace any invalid characters with underscore
            foreach(var c in Path.GetInvalidPathChars()) {
                path.Replace(c, '_');
            }
            foreach (var c in Path.GetInvalidFileNameChars()) {
                file.Replace(c, '_');
            }

            return path.Append(file).ToString();
        }

        public static byte[]? ComputeHash(byte[]? data, int offset = 0, int count = 0)
        {
            if(data?.Length > 0) {
                using var sha256 = SHA256.Create();
                return sha256.ComputeHash(data, offset, count > 0 ? count : data.Length);
            }
            return null;
        }

        public static async Task ImportFilesAsync(string[] files, string destinationFolder)
        {
            try {
                Debug.Assert(!string.IsNullOrEmpty(destinationFolder));
                // disable ContentWatcher's events to avoid unnecessary updates to AssetRegistery and ContentBrowser
                ContentWatcher.EnableFileWatcher(false);

                // send all import tasks and await until all of them are finished
                var tasks = files.Select(async file => await Task.Run(() =>
                {
                    Import(file, destinationFolder);
                }));
                await Task.WhenAll(tasks);
            }
            catch (Exception ex) {
                Debug.WriteLine($"Failed to import files to {destinationFolder}");
                Debug.WriteLine(ex.Message);
            }
            finally {
                // enables regardless of any errors that occured. (last thing we want is to have files broken in our game's project dir
                ContentWatcher.EnableFileWatcher(true);
            }
        }

        private static void Import(string file, string destinationFolder)
        {
            Debug.Assert(!string.IsNullOrEmpty(file));
            if (IsDirectory(file)) return;
            if(!destinationFolder.EndsWith(Path.DirectorySeparatorChar)) destinationFolder += Path.DirectorySeparatorChar;

            var name = Path.GetFileNameWithoutExtension(file).ToLower();
            var ext = Path.GetExtension(file).ToLower();

            Asset? asset = null;
            switch (ext) {
                case ".fbx": {
                        asset = new Content.Geometry();
                        break;
                    }
                case ".bmp": break;
                case ".png": break;
                case ".jpg": break;
                case ".jpeg": break;
                case ".tiff": break;
                case ".tif": break;
                case ".tga": break;
                case ".wav": break;
                case ".ogg": break;
                default:
                    break;
            }

            if(asset != null) {
                Import(asset, name, file, destinationFolder);
            }
        }

        private static void Import(Asset asset, string name, string file, string destinationFolder)
        {
            Debug.Assert(asset != null);

            asset.FullPath = destinationFolder + name + Asset.AssetFileExtension;
            // well technically we assert such thing in the method above called by async Task Import FilesAsync(), but
            // just to be sure, if someone called it somewhere else and didn't know file path can't be null or empty.
            if(!string.IsNullOrEmpty(file)) {
                asset.Import(file);
            }

            asset.Save(asset.FullPath);
            return;
        }
    }
}
