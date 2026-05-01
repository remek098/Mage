using MageEditor.GameProject;
using MageEditor.Utilities;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
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

namespace MageEditor.GameDev
{
    /// <summary>
    /// Interaction logic for NewScriptDialog.xaml
    /// </summary>
    public partial class NewScriptDialog : Window
    {
        private static readonly string _cppCode = @"#include ""{0}.h""

namespace {1} {{

    REGISTER_SCRIPT({0});
    void {0}::begin_play() {{
        
    }}

    void {0}::update(float dt) {{
        
    }}
}} // namespace {1}";


        private static readonly string _hCode = @"#pragma once

namespace {1} {{
    
    class {0} : public mage::script::ScriptEntity {{
    public:
        constexpr explicit {0}(mage::game_entity::Entity entity)
            : mage::script::ScriptEntity(entity) {{}}

        void begin_play() override;
        void update(float dt) override;
    
    private:
    }};
    
}} // namespace {1}";

        private static readonly string _namespace = GetNamespaceFromProjectName();

        private static string GetNamespaceFromProjectName()
        {
            var project_name = Project.Current?.Name;
            if (string.IsNullOrEmpty(project_name)) return string.Empty; // basically checking to get annoying warnings away
                                                                         // if that was the case, then I guess the code for our script would reside within anonymous namespace
                                                                         // but it shouldn't happen since Project.Current is defined as follows in Project.cs
                                                                         // public static Project? Current => Application.Current.MainWindow.DataContext as Project;
                                                                         // and also if Project.Current would be null, that means we never loaded any project
                                                                         // so we aren't even able to be there in first place.
            project_name = project_name.Replace(' ', '_'); // replace spaces with underscore
            return project_name;
        }

        bool Validate()
        {
            bool isValid = false;
            var name = scriptName.Text.Trim();
            var path = scriptPath.Text.Trim();
            string errorMsg = string.Empty;

            if(string.IsNullOrEmpty(name))
            {
                errorMsg = "Type in a script name.";
            }
            else if (name.IndexOfAny(Path.GetInvalidFileNameChars()) != -1 || name.Any(x => char.IsWhiteSpace(x)))
            {
                errorMsg = "Invalid character(s) used in script name.";
            }
            else if (string.IsNullOrEmpty(path))
            {
                errorMsg = "Select valid script folder.";
            }
            else if (path.IndexOfAny(Path.GetInvalidPathChars()) != -1)
            {
                errorMsg = "Invalid character(s) used in script's path.";
            }
            else if (Project.Current != null && !Path.GetFullPath(Path.Combine(Project.Current.Path, path)).Contains(Path.Combine(Project.Current.Path, @"GameCode\")))
            {
                errorMsg = "Script must be added to GameCode folder (or subfolder of GameCode folder.";
            }
            else if (Project.Current != null &&  ( File.Exists(Path.GetFullPath(Path.Combine(Path.Combine(Project.Current.Path, path), $"{name}.cpp"))) ||
                File.Exists(Path.GetFullPath(Path.Combine(Path.Combine(Project.Current.Path, path), $"{name}.h"))) ) )
            {
                errorMsg = $"Script {name} already exists in this folder.";
            }
            else
            {
                isValid = true;
            }

            if(!isValid)
            {
                messageTextBlock.Foreground = FindResource("Editor.RedBrush") as Brush;
            }
            else
            {
                messageTextBlock.Foreground = FindResource("Editor.FontBrush") as Brush;
            }

            messageTextBlock.Text = errorMsg;
            return isValid;

        }

        private void OnScriptName_TextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            if (!Validate()) return;

            var name = scriptName.Text.Trim();
            if (Project.Current != null) messageTextBlock.Text = $"{name}.h and {name}.cpp will be added to {Project.Current.Name}";
        }

        private void OnScriptPath_TextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            Validate();
        }

        private async void OnCreate_Button_Click(object sender, RoutedEventArgs e)
        {
            if (!Validate()) return;
            IsEnabled = false;

            // make our nice waiting animation visible
            busyAnimation.Opacity = 0;
            busyAnimation.Visibility = Visibility.Visible;
            DoubleAnimation fadeIn = new DoubleAnimation(0, 1, new Duration(TimeSpan.FromMilliseconds(500)));
            busyAnimation.BeginAnimation(OpacityProperty, fadeIn);

            try
            {
                if (Project.Current == null) return;

                // cache values locally because it will run on seperate thread
                // -> just to have access within thread to viable information that could
                // otherwise just disappear for no good reason :-)
                var name = scriptName.Text.Trim();
                var path = Path.GetFullPath(Path.Combine(Project.Current.Path, scriptPath.Text.Trim()));
                var solution = Project.Current.Solution;
                var project_name = Project.Current.Name;

                await Task.Run(() => CreateScript(name, path, solution, project_name));
            }
            catch (Exception ex) 
            {
                Debug.WriteLine(ex.Message);
                Logger.Log(MessageType.Error, $"Failed to create script {scriptName.Text}");
            }
            finally
            {
                DoubleAnimation fadeOut = new DoubleAnimation(1, 0, new Duration(TimeSpan.FromMilliseconds(300)));
                fadeOut.Completed += (s, e) => 
                {
                    busyAnimation.Opacity = 0;
                    busyAnimation.Visibility = Visibility.Hidden;
                    Close(); // close dialog when fadeOut animation will stop playing.
                };
                busyAnimation.BeginAnimation(OpacityProperty, fadeOut);
            }
        }

        private void CreateScript(string name, string path, string solution, string projectName)
        {
            if (!Directory.Exists(path)) Directory.CreateDirectory(path);

            var cpp_file = Path.GetFullPath(Path.Combine(path, $"{name}.cpp"));
            var h_file = Path.GetFullPath(Path.Combine(path, $"{name}.h"));


            // create/write .cpp and .h files in utf-8 text
            using(var sw = File.CreateText(cpp_file)) 
            {
                sw.Write(string.Format(_cppCode, name, _namespace));
            }

            using (var sw = File.CreateText(h_file))
            {
                sw.Write(string.Format(_hCode, name, _namespace));
            }

            string[] files = new string[] { cpp_file, h_file };
            for(int i = 0; i < 3; ++i)
            {
                if (!VisualStudio.AddFilesToSolution(solution, projectName, files)) System.Threading.Thread.Sleep(1000);
                else break;
            }

        }

        public NewScriptDialog()
        {
            InitializeComponent();
            Owner = Application.Current.MainWindow;
            scriptPath.Text = @"GameCode\";
        }
    }
}
