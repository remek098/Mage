using MageEditor.Utilities;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;

namespace MageEditor.GameProject
{

    [DataContract]
    public class ProjectData
    {
        [DataMember]
        public string ProjectName { get; set; }
        
        [DataMember]
        public string ProjectPath { get; set; }

        [DataMember]
        public DateTime Date {  get; set; }

        public string FullPath { get => $"{ProjectPath}{ProjectName}{Project.Extension}"; }

        public byte[] Icon { get; set; }

        // aka screenshot
        public byte[] TemplateImage { get; set; }
    }

    [DataContract]
    public class ProjectDataList
    {
        [DataMember]
        public List<ProjectData> Projects { get; set; }
    }

    class OpenProject
    {

        // that's the place where we want to save our data about available to open projects. (%AppData%\MageEditor\)
        private static readonly string _applicationDataPath = $@"{Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData)}\MageEditor\";

        private static readonly string _projectDataPath;

        private static readonly ObservableCollection<ProjectData> _projects = new ObservableCollection<ProjectData>();

        public static ReadOnlyObservableCollection<ProjectData> Projects { get; }

        private static void ReadProjectData()
        {
            if(File.Exists(_projectDataPath))
            {
                var project_data_list = Serializer.FromFile<ProjectDataList>(_projectDataPath);
                if (project_data_list != null)
                {
                    var projects = project_data_list.Projects.OrderByDescending(x => x.Date);


                    _projects.Clear();
                    foreach (var project in projects)
                    {
                        if (File.Exists(project.FullPath))
                        {
                            project.Icon = File.ReadAllBytes($@"{project.ProjectPath}\.Mage\Icon.png");
                            project.TemplateImage = File.ReadAllBytes($@"{project.ProjectPath}\.Mage\TemplateImage.png");
                            _projects.Add(project);
                        }
                    }
                }
            }
        }

        private static void WriteProjectData()
        {
            var projects = _projects.OrderBy(x => x.Date).ToList();
            Serializer.ToFile(new ProjectDataList() { Projects = projects }, _projectDataPath);
        }

        public static Project? Open(ProjectData data)
        {
            ReadProjectData();
            var project = _projects.FirstOrDefault(x => x.FullPath == data.FullPath);
            
            if(project != null)
            {
                // means we found one in a list
                project.Date = DateTime.Now;
            }
            else
            {
                // data was not in the list
                project = data;
                project.Date = DateTime.Now;
                _projects.Add(project);
            }

            WriteProjectData();

            return Project.Load(project.FullPath);
        }

        static OpenProject()
        {
            try
            {
                if(!Directory.Exists(_applicationDataPath)) Directory.CreateDirectory(_applicationDataPath);
                _projectDataPath = $@"{_applicationDataPath}ProjectData.xml";
                Projects = new ReadOnlyObservableCollection<ProjectData>(_projects);
                ReadProjectData();
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
                Logger.Log(MessageType.Error, $"Failed to read project data");
                throw;
            }
        }
    }
}
