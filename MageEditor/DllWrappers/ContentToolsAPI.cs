using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Numerics;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using MageEditor.ContentToolsAPIStructs;
using MageEditor.Utilities;



namespace MageEditor.ContentToolsAPIStructs
{
    [StructLayout(LayoutKind.Sequential)]
    class GeometryImportSettings
    {
        public float SmoothingAngle         = 178f;
        public byte CalculateNormals        = 0;
        public byte CalculateTangents       = 1;
        public byte ReverseHandedness       = 0;
        public byte ImportEmbededTextures   = 1;
        public byte ImportAnimations        = 1;
        private byte ToByte(bool value) => value ? (byte)1 : (byte)0;

        public void FromContentSettings(Content.Geometry geometry)
        {
            // copy settings from Content.Geometry class that editor sets up
            var settings = geometry.ImportSettings;
            SmoothingAngle = settings.SmoothingAngle;
            CalculateNormals = ToByte(settings.CalculateNormals);
            CalculateTangents = ToByte(settings.CalculateTangents);
            ReverseHandedness = ToByte(settings.ReverseHandedness);
            ImportEmbededTextures = ToByte(settings.ImportEmbededTextures);
            ImportAnimations = ToByte(settings.ImportAnimations);
        }

    }


    [StructLayout(LayoutKind.Sequential)]
    class SceneData : IDisposable
    {
        public IntPtr Data;
        public int DataSize;
        public GeometryImportSettings ImportSettings = new GeometryImportSettings();

        public void Dispose()
        {
            Marshal.FreeCoTaskMem(Data);
            GC.SuppressFinalize(this); // supress finalizer (called by GC)
        }

        ~SceneData()
        {
            Dispose();
        }
    }


    // mapping for engine's primitive_mesh_init_info struct
    [StructLayout(LayoutKind.Sequential)]
    class PrimitiveInitInfo
    {
        public Content.PrimitiveMeshType Type;
        public int SegmentX = 1;
        public int SegmentY = 1;
        public int SegmentZ = 1;
        public Vector3 Size = new Vector3(1f);
        int LOD = 0;
    }
}

namespace MageEditor.DllWrappers
{
    static class ContentToolsAPI
    {
        private const string _toolsDll = "ContentTools.dll";


        // refactor tractor for no particular reason
        private static void GeometryFromSceneData(Content.Geometry geometry, Action<SceneData> sceneDataGenerator, string failureMessage)
        {
            Debug.Assert(geometry != null);
            using var sceneData = new SceneData(); // this will make sure to call destructor (which frees CoTaskMem allocated
            // in c++ side of engine (inside mage::tools::pack_data()  )
            try {
                sceneData.ImportSettings.FromContentSettings(geometry);
                sceneDataGenerator(sceneData); // dll's function
                // if packing of mesh's data was successful, that assert below should pass
                Debug.Assert(sceneData.Data != IntPtr.Zero && sceneData.DataSize > 0);
                var data = new byte[sceneData.DataSize];
                // copy data from scene.Data (that was allocated by CoTaskMemAlloc() (inside mage::tools::pack_data())
                // into our byte[] data (all of it, from 0 to end of buffer)
                Marshal.Copy(sceneData.Data, data, 0, sceneData.DataSize);
                // Marshal.FreeCoTaskMem(sceneData.Data); // one way of doing it
                // -> but it's done automatically by using var sceneData, which makes sure to call destructor

                geometry.FromRawData(data);
            }
            catch (Exception ex) {
                Logger.Log(MessageType.Error, failureMessage);
                Debug.WriteLine(ex.Message);
            }
        }

        [DllImport(_toolsDll)]
        private static extern void CreatePrimitiveMesh([In, Out] SceneData data, PrimitiveInitInfo info);

        public static void CreatePrimitiveMesh(Content.Geometry geometry, PrimitiveInitInfo info)
        {
            GeometryFromSceneData(geometry, (sceneData) => CreatePrimitiveMesh(sceneData, info), 
                $"Failed to create {info.Type} primitive mesh.");
        }


        // similar to CreatePrimitiveMesh() dll import
        [DllImport(_toolsDll)]
        private static extern void ImportFbx(string file, [In, Out] SceneData data);
        public static void ImportFbx(string file, Content.Geometry geometry)
        {
            GeometryFromSceneData(geometry, (sceneData) => ImportFbx(file, sceneData),
                $"Failed to import from FBX file: {file}");
        }
    }
}
