using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
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
    }
}
