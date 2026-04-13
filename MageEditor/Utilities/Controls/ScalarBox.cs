using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;

namespace MageEditor.Utilities.Controls
{
    class ScalarBox : NumberBox
    {
        static ScalarBox()
        {
            // https://learn.microsoft.com/en-us/dotnet/api/system.windows.frameworkelement.defaultstylekey?view=windowsdesktop-10.0
            // custom style key, so that WPF knows what to apply.
            DefaultStyleKeyProperty.OverrideMetadata(typeof(ScalarBox), new FrameworkPropertyMetadata(typeof(ScalarBox)));
        }
    }
}
