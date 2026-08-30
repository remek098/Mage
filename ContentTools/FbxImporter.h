#pragma once
#include "ToolsCommon.h"
#include <fbxsdk.h>

namespace mage::tools {
    // forward declerations of structs from Geometry.h
    struct scene_data;
    struct scene;
    struct mesh;
    struct geometry_import_settings;


    //class fbx_context {
    //public:
    //    fbx_context(const char* file, scene* scene, scene_data* data) 
    //        : _scene{ scene }, _scene_data{ data } 
    //    {
    //        assert(file && _scene && _scene_data);
    //        if (initialize_fbx()) {
    //            load_fbx_file(file);
    //            assert(is_valid());
    //        }
    //    }

    //    ~fbx_context() {
    //        _fbx_scene->Destroy();
    //        _fbx_manager->Destroy();
    //        ZeroMemory(this, sizeof(fbx_context));
    //    }

    //    void get_scene(FbxNode* root = nullptr);

    //    constexpr bool is_valid() const {return _fbx_manager && _fbx_scene; }
    //    constexpr f32 scene_scale() const {return _scene_scale; }

    //private:
    //    bool initialize_fbx();
    //    void load_fbx_file(const char* file);

    //    //void get_meshes(FbxNode* node, utl::vector<mesh>& meshes, u32 lod_id, f32 lod_treshold);
    //    //// meshes is out param where we get engine's format of this mesh added in.
    //    //void get_mesh(FbxNodeAttribute* attribute, utl::vector<mesh>& meshes, u32 lod_id, f32 lod_treshold);
    //    void get_mesh(
    //        FbxNodeAttribute* attribute,
    //        const std::string& group_name,
    //        u32 lod_id,
    //        f32 lod_threshold);
    //    void get_meshes(
    //        FbxNode* node,
    //        const std::string& group_name,
    //        u32 lod_id,
    //        f32 lod_threshold);

    //    void get_mesh(FbxNodeAttribute* attribute);
    //    void get_meshes(FbxNode* node);
    //    void get_lod_group(FbxNodeAttribute* attribute);
    //    bool get_mesh_data(FbxMesh* fbx_mesh, mesh& m);

    //private:
    //    scene*                      _scene = nullptr;
    //    scene_data*                 _scene_data = nullptr;
    //    FbxManager*                 _fbx_manager = nullptr;
    //    FbxScene*                   _fbx_scene = nullptr;

    //    // since each file can use diffrent scale, we'll need to retrieve scale
    //    // and convert it to meters so that all vertex positions or anything else that has
    //    // to do with distance or position uses same units in our engine.
    //    f32                         _scene_scale = 1.0f;
    //};
}