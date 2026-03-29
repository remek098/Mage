using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;

namespace MageEditor.GameProject
{
    /// <summary>
    /// Interaction logic for ProjectBrowserDialog.xaml
    /// </summary>
    public partial class ProjectBrowserDialog : Window
    {
        private readonly CubicEase _animEasing = new CubicEase() { EasingMode = EasingMode.EaseInOut };

        // https://learn.microsoft.com/en-us/dotnet/desktop/wpf/graphics-multimedia/easing-functions
        public ProjectBrowserDialog()
        {
            InitializeComponent();
            Loaded += OnProjectBrowserDialogLoaded;
        }

        private void OnProjectBrowserDialogLoaded(object sender, RoutedEventArgs e)
        {
            Loaded -= OnProjectBrowserDialogLoaded;
            if(!OpenProject.Projects.Any())
            {
                openProjectButton.IsEnabled = false;
                openProjectView.Visibility = Visibility.Hidden;
                // it's basically like user clicked Create Project button by default if there's no projects to load
                OnToggleButton_Click(createProjectButton, new RoutedEventArgs());
            }
        }


        private void AnimateToCreateProject()
        {
            // https://learn.microsoft.com/en-us/dotnet/api/system.windows.media.animation.doubleanimation?view=windowsdesktop-10.0
            // https://learn.microsoft.com/en-us/dotnet/api/system.windows.media.animation.thicknessanimation?view=windowsdesktop-10.0
            var highlightAnim = new DoubleAnimation(200, 420, new Duration(TimeSpan.FromSeconds(0.2)));
            highlightAnim.EasingFunction = _animEasing;
            highlightAnim.Completed += (s, e) =>
            {
                var anim = new ThicknessAnimation(new Thickness(0), new Thickness(-800, 0, 0, 0), new Duration(TimeSpan.FromSeconds(0.5)));
                anim.EasingFunction = _animEasing;
                browserContent.BeginAnimation(MarginProperty, anim);
            };
            highlightRect.BeginAnimation(Canvas.LeftProperty, highlightAnim);
        }

        private void AnimateToOpenProject()
        {
            // https://learn.microsoft.com/en-us/dotnet/api/system.windows.media.animation.doubleanimation?view=windowsdesktop-10.0
            // https://learn.microsoft.com/en-us/dotnet/api/system.windows.media.animation.thicknessanimation?view=windowsdesktop-10.0
            var highlightAnim = new DoubleAnimation(420, 200, new Duration(TimeSpan.FromSeconds(0.2)));
            highlightAnim.EasingFunction = _animEasing;
            highlightAnim.Completed += (s, e) =>
            {
                var anim = new ThicknessAnimation(new Thickness(-800, 0, 0, 0), new Thickness(0), new Duration(TimeSpan.FromSeconds(0.5)));
                anim.EasingFunction = _animEasing;
                browserContent.BeginAnimation(MarginProperty, anim);
            };
            highlightRect.BeginAnimation(Canvas.LeftProperty, highlightAnim);
        }


        private void OnToggleButton_Click(object sender, RoutedEventArgs e)
        {
            if(sender == openProjectButton)
            {
                if(createProjectButton.IsChecked == true)
                {
                    createProjectButton.IsChecked = false;
                    AnimateToOpenProject();
                    openProjectView.IsEnabled = true;
                    newProjectView.IsEnabled = false;
                    // browserContent.Margin = new Thickness(0);
                }
                openProjectButton.IsChecked = true;
            }
            else
            {
                if (openProjectButton.IsChecked == true)
                {
                    openProjectButton.IsChecked = false;
                    AnimateToCreateProject();
                    openProjectView.IsEnabled = false;
                    newProjectView.IsEnabled = true;
                    //browserContent.Margin = new Thickness(-800, 0, 0, 0);
                }
                createProjectButton.IsChecked = true;
            }
        }

    }
}
