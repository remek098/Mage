using MageEditor.GameProject;
using System.ComponentModel;
using System.IO;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;

namespace MageEditor
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        // NOTE: to be changed, Engine will be installed where user chooses ideally.
        public static string MagePath { get; private set; } // = @"F:\dev\Mage\Mage_git\Mage";

        public MainWindow()
        {
            InitializeComponent();
            Loaded += OnMainWindowLoaded;
            Closing += OnMainWindowClosing;
        }

        

        private void OnMainWindowLoaded(object sender, RoutedEventArgs e)
        {
            Loaded -= OnMainWindowLoaded;
            GetEnginePath();
            OpenProjectBrowserDialog();
        }

        private void GetEnginePath()
        {
            // if this environment variable was not set, returns null
            var magePath = Environment.GetEnvironmentVariable("MAGE_ENGINE", EnvironmentVariableTarget.User);
            if(magePath == null || !Directory.Exists(Path.Combine(magePath, @"Engine\EngineAPI")))
            {
                // close application if we weren't able to receive a path to where engine is installed
                var dlg = new EnginePathDialog();
                if(dlg.ShowDialog() == true)
                {
                    MagePath = dlg.MagePath;
                    Environment.SetEnvironmentVariable("MAGE_ENGINE", MagePath.ToUpper(), EnvironmentVariableTarget.User);
                }
                else
                {
                    Application.Current.Shutdown();
                }
            }
            else
            {
                MagePath = magePath;
            }
        }

        private void OnMainWindowClosing(object? sender, CancelEventArgs e)
        {
            Closing -= OnMainWindowClosing;
            Project.Current?.Unload();
        }

        private void OpenProjectBrowserDialog()
        {
            var projectBrowser = new ProjectBrowserDialog();
            projectBrowser.Owner = this;
            // for some reason we failed to open a project if DataContext is null
            if (projectBrowser.ShowDialog() == false || projectBrowser.DataContext == null)
            {
                Application.Current.Shutdown();
            }
            else 
            {
                Project.Current?.Unload();
                DataContext = projectBrowser.DataContext;
            }
        }
    }
}