using System;
using System.Collections.Generic;
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
    }
}
