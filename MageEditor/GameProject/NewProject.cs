using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using System.Diagnostics;
using System.Runtime.Serialization;
using MageEditor.Utilities;
using System.Collections.ObjectModel;

namespace MageEditor.GameProject
{
    [DataContract]
    public class ProjectTemplate
    {
        [DataMember]
        public string ProjectType { get; set; }

        [DataMember]
        public string ProjectFile { get; set; }

        [DataMember]
        public List<string> Folders { get; set; }

        public byte[] Icon { get; set; }

        public byte[] TemplateImage { get; set; }

        public string IconFilePath { get; set; }
        public string TemplateImageFilePath { get; set; }

        public string ProjectFilePath { get; set; }
    }

    class NewProject : ViewModelBase
    {
        // TODO: get the path from the installation location
        private readonly string _templatePath = @"..\..\MageEditor\ProjectTemplates";
        private string _projectName = "NewProject";
        public string ProjectName
        {
            get => _projectName;
            set
            {
                if(_projectName != value)
                {
                    _projectName = value;
                    ValidateProjectPath();
                    OnPropertyChanged(nameof(ProjectName));
                }
            }
        }

        private string _projectPath = $@"{Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments)}\MageProjects\";
        public string ProjectPath
        {
            get => _projectPath;
            set
            {
                if (_projectPath != value)
                {
                    _projectPath = value;
                    ValidateProjectPath();
                    OnPropertyChanged(nameof(ProjectPath));
                }
            }
        }

        private bool _IsValid;
        public bool IsValid
        {
            get => _IsValid;
            set
            {
                if (_IsValid != value)
                {
                    _IsValid = value; ;
                    OnPropertyChanged(nameof(IsValid));
                }
            }
        }

        private string _errorMsg;
        public string ErrorMsg
        {
            get => _errorMsg;
            set
            {
                if (_errorMsg != value)
                {
                    _errorMsg = value;
                    OnPropertyChanged(nameof(ErrorMsg));
                }
            }
        }

        private ObservableCollection<ProjectTemplate?> _projectTemplates = new ObservableCollection<ProjectTemplate?>();
        public ReadOnlyObservableCollection<ProjectTemplate?> ProjectTemplates { get; }

        private bool ValidateProjectPath()
        {
            var path = ProjectPath;
            if(!Path.EndsInDirectorySeparator(path)) path += @"\";
            path += $@"{ProjectName}\";

            IsValid = false;
            if (string.IsNullOrWhiteSpace(ProjectName.Trim()))
            {
                ErrorMsg = "Type in a project name.";
            }
            else if (ProjectName.IndexOfAny(Path.GetInvalidFileNameChars()) != -1)
            {
                ErrorMsg = "Invalid character*(s) in project's name.";
            }
            else if (string.IsNullOrWhiteSpace(ProjectPath.Trim()))
            {
                ErrorMsg = "Select a valid project folder.";
            }
            else if (ProjectPath.IndexOfAny(Path.GetInvalidPathChars()) != -1)
            {
                ErrorMsg = "Invalid character*(s) in project's path.";
            }
            else if (Path.Exists(path) && Directory.EnumerateFileSystemEntries(path).Any())
            {
                ErrorMsg = "Selected project folder already exists and is not empty.";
            }
            else
            {
                ErrorMsg = string.Empty;
                IsValid = true;
            }

            return IsValid;


        }

        public string CreateProject(ProjectTemplate? template)
        {
            ValidateProjectPath();
            if(!IsValid || template == null)
            {
                return string.Empty;
            }

            // fix the project's path
            if (!Path.EndsInDirectorySeparator(ProjectPath)) ProjectPath += @"\";
            var path = $@"{ProjectPath}{ProjectName}\";

            try
            {
                if(!Directory.Exists(path)) Directory.CreateDirectory(path);
                // doing this because we got nullable types
                // if(template.Folders != null && template.IconFilePath != null)
                // {
                var basePath = Path.GetDirectoryName(path) ?? path;
                foreach (var folder in template.Folders)
                {
                    Directory.CreateDirectory(Path.GetFullPath(Path.Combine(basePath, folder)));
                }
                var dirInfo = new DirectoryInfo(path + @".Mage\");
                dirInfo.Attributes |= FileAttributes.Hidden; // make sure folder is hidden
                File.Copy(template.IconFilePath, Path.GetFullPath(Path.Combine(dirInfo.FullName, "Icon.png")));
                File.Copy(template.TemplateImageFilePath, Path.GetFullPath(Path.Combine(dirInfo.FullName, "TemplateImage.png")));


                // }
                //var project = new Project(ProjectName, path);
                //Serializer.ToFile(project, path + $"{ProjectName}" + Project.Extension);

                // .mage file in template has {0} and {1} string formats that we utilize there
                var projectXml = File.ReadAllText(template.ProjectFilePath); 
                projectXml = string.Format(projectXml, ProjectName, ProjectPath); // maybe instead of ProjectPath we got to use path variable itself -> depends if you want later when reading a file append ProjectName like path variable does
                var projectPath = Path.GetFullPath(Path.Combine(path, $"{ProjectName}{Project.Extension}"));
                File.WriteAllText(projectPath, projectXml);
                return path;
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
                Logger.Log(MessageType.Error, $"Failed to create {ProjectName}.");
                throw;
            }
        }

        public NewProject()
        {
            ProjectTemplates = new ReadOnlyObservableCollection<ProjectTemplate?>(_projectTemplates);

            try
            {
                var templatesFiles = Directory.GetFiles(_templatePath, "template.xml", SearchOption.AllDirectories);
                Debug.Assert(templatesFiles.Any());
                foreach (var file in templatesFiles)
                {
                    var template = Serializer.FromFile<ProjectTemplate>(file);
                    var directory = Path.GetDirectoryName(file);

                    if (directory == null)
                        continue;

                    if (template != null)
                    {
                        template.IconFilePath = Path.GetFullPath(Path.Combine(directory, "Icon.png"));
                        template.Icon = File.ReadAllBytes(template.IconFilePath);

                        template.TemplateImageFilePath = Path.GetFullPath(Path.Combine(directory, "TemplateImage.png"));
                        template.TemplateImage = File.ReadAllBytes(template.TemplateImageFilePath);

                        if (template.ProjectFile != null)
                            template.ProjectFilePath = Path.GetFullPath(Path.Combine(directory, template.ProjectFile));
                    }
                    _projectTemplates.Add(template);
                    
                }
                ValidateProjectPath();
            }
            catch(Exception ex) 
            {
                Debug.WriteLine(ex.Message);
                Logger.Log(MessageType.Error, $"Failed to read project templates.");
                throw;
            }
        }
    }
}
