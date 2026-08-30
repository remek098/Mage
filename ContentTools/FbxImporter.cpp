#include "FbxImporter.h"

#include "Geometry.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <string>
#include <algorithm>
#include <regex>

namespace mage::tools {

namespace {
    std::mutex fbx_mutex{};
} // anonymous namespace

    //bool fbx_context::initialize_fbx() {
    //    // https://help.autodesk.com/view/FBX/2020/ENU/?guid=FBX_Developer_Help_getting_started_your_first_fbx_sdk_program_html
    //    assert(!is_valid());
    //    _fbx_manager = FbxManager::Create();
    //    if(!_fbx_manager) return false;

    //    // https://help.autodesk.com/view/FBX/2020/ENU/?guid=FBX_Developer_Help_importing_and_exporting_a_scene_io_settings_html
    //    FbxIOSettings* ios = FbxIOSettings::Create(_fbx_manager, IOSROOT);
    //    _fbx_manager->SetIOSettings(ios);

    //    return true;
    //}

    //void fbx_context::load_fbx_file(const char* file) {
    //    assert(_fbx_manager && !_fbx_scene);
    //    _fbx_scene = FbxScene::Create(_fbx_manager, "Importer Scene");
    //    if(!_fbx_scene) return;

    //    FbxImporter* importer = FbxImporter::Create(_fbx_manager, "Importer");
    //    if (!(importer &&
    //          importer->Initialize(file, -1, _fbx_manager->GetIOSettings()) && 
    //          importer->Import(_fbx_scene))) {
    //        // on failure to create, initialize or import
    //        if(importer) importer->Destroy();
    //        return;
    //    }
    //    importer->Destroy();

    //    // get scene scale in meters.
    //    _scene_scale = (f32)_fbx_scene->GetGlobalSettings().GetSystemUnit().GetConversionFactorTo(FbxSystemUnit::m);
    //}


    //

    //void fbx_context::get_scene(FbxNode* root /* = nullptr */) {
    //    assert(is_valid());
    //    // https://help.autodesk.com/view/FBX/2020/ENU/?guid=FBX_Developer_Help_nodes_and_scene_graph_fbx_scenes_html
    //    // https://help.autodesk.com/view/FBX/2020/ENU/?guid=FBX_Developer_Help_nodes_and_scene_graph_fbx_nodes_html
    //    if (!root) {
    //        root = _fbx_scene->GetRootNode();
    //        if (!root) return;
    //    }
    //    get_meshes(root);

    //    // fill in defaults only for meshes that don't already have tresholds.
    //    _scene->generate_default_lod_thresholds();
    //}

    //void fbx_context::get_mesh(
    //                           FbxNodeAttribute* attribute,
    //                           const std::string& group_name,
    //                           u32 lod_id,
    //                           f32 lod_threshold) {
    //    assert(attribute);

    //    FbxMesh* fbx_mesh = static_cast<FbxMesh*>(attribute);
    //    if (fbx_mesh->RemoveBadPolygons() < 0) return;

    //    FbxGeometryConverter gc{ _fbx_manager };
    //    fbx_mesh = static_cast<FbxMesh*>(gc.Triangulate(fbx_mesh, true));
    //    if (!fbx_mesh || fbx_mesh->RemoveBadPolygons() < 0) return;

    //    mesh m{};
    //    m.name = group_name;
    //    m.lod_id = lod_id;
    //    m.lod_treshhold = lod_threshold;

    //    if (!get_mesh_data(fbx_mesh, m)) return;

    //    _scene->add_mesh(std::move(m));
    //}

    //void fbx_context::get_meshes(
    //                             FbxNode* node,
    //                             const std::string& group_name,
    //                             u32 lod_id,
    //                             f32 lod_threshold) {
    //    assert(node);

    //    const i32 attribute_count = node->GetNodeAttributeCount();

    //    for (i32 i = 0; i < attribute_count; ++i) {
    //        FbxNodeAttribute* attribute =
    //            node->GetNodeAttributeByIndex(i);

    //        if (!attribute)
    //            continue;

    //        if (attribute->GetAttributeType() ==
    //            FbxNodeAttribute::eMesh) {
    //            get_mesh(attribute,
    //                     group_name,
    //                     lod_id,
    //                     lod_threshold);
    //        }
    //    }

    //    const i32 child_count = node->GetChildCount();

    //    for (i32 i = 0; i < child_count; ++i) {
    //        get_meshes(node->GetChild(i),
    //                   group_name,
    //                   lod_id,
    //                   lod_threshold);
    //    }
    //}


    //void fbx_context::get_meshes(FbxNode* node) {
    //    assert(node);
    //    bool is_lod_group = false;

    //    const i32 attribute_count = node->GetNodeAttributeCount();
    //    for (i32 i = 0; i < attribute_count; ++i) {
    //        FbxNodeAttribute* attribute = node->GetNodeAttributeByIndex(i);

    //        if (!attribute) continue;

    //        switch (attribute->GetAttributeType()) {
    //            case FbxNodeAttribute::eMesh:
    //                get_mesh(attribute);
    //                break;

    //            case FbxNodeAttribute::eLODGroup:
    //                get_lod_group(attribute);
    //                is_lod_group = true;
    //                return;
    //        }
    //    }

    //    if (!is_lod_group) {
    //        const i32 child_count = node->GetChildCount();
    //        for (i32 i = 0; i < child_count; ++i) {
    //            get_meshes(node->GetChild(i));
    //        }
    //    }
    //}

    //void fbx_context::get_mesh(FbxNodeAttribute* attribute) {
    //    assert(attribute);

    //    FbxMesh* fbx_mesh = static_cast<FbxMesh*>(attribute);
    //    if (fbx_mesh->RemoveBadPolygons() < 0) return;

    //    FbxGeometryConverter gc{ _fbx_manager };
    //    fbx_mesh = static_cast<FbxMesh*>(gc.Triangulate(fbx_mesh, true));
    //    if (!fbx_mesh || fbx_mesh->RemoveBadPolygons() < 0) return;

    //    mesh m{};
    //    FbxNode* node = fbx_mesh->GetNode();

    //    m.name = node->GetName()[0] ?
    //        node->GetName() :
    //        fbx_mesh->GetName();

    //    if (!get_mesh_data(fbx_mesh, m)) return;

    //    auto info = parse_lod_name(m.name);
    //    if (info.is_lod) {
    //        m.name = info.base_name;
    //        m.lod_id = info.lod_id;
    //    }
    //    else {
    //        m.lod_id = 0;
    //    }

    //    _scene->add_mesh(std::move(m));
    //}



    //void fbx_context::get_lod_group(FbxNodeAttribute* attribute) {
    //    assert(attribute);
    //    // https://help.autodesk.com/view/FBX/2015/ENU/?guid=__cpp_ref_class_fbx_l_o_d_group_html -> for some reason it's referenced in FBX 2015 doc, but not for 2020
    //    auto* lod_grp = static_cast<FbxLODGroup*>(attribute);
    //    FbxNode* node = lod_grp->GetNode();

    //    const std::string group_name = node->GetName()[0] ? 
    //        node->GetName() :
    //        lod_grp->GetName();

    //    // NOTE: number of LODs is exclusive to the base mesh (LOD0)
    //    const i32 child_count = node->GetChildCount();

    //    assert(child_count > 0);

    //    for (i32 i = 0; i < child_count; ++i) {
    //        f32 lod_threshold = -1.f;

    //        if (i > 0) {
    //            FbxDistance distance;
    //            lod_grp->GetThreshold(i - 1, distance);

    //            lod_threshold =
    //                distance.value() * _scene_scale;
    //        }

    //        get_meshes(node->GetChild(i), group_name, (u32)i, lod_threshold);
    //    }
    //}

    //bool fbx_context::get_mesh_data(FbxMesh* fbx_mesh, mesh& m) {
    //    assert(fbx_mesh);

    //    FbxNode* const node = fbx_mesh->GetNode();
    //    FbxAMatrix geo_transform_mat;

    //    geo_transform_mat.SetT(node->GetGeometricTranslation(FbxNode::eSourcePivot));
    //    geo_transform_mat.SetR(node->GetGeometricRotation(FbxNode::eSourcePivot));
    //    geo_transform_mat.SetS(node->GetGeometricScaling(FbxNode::eSourcePivot));

    //    FbxAMatrix transform{node->EvaluateGlobalTransform() * geo_transform_mat};
    //    FbxAMatrix inverse_transpose{transform.Inverse().Transpose()};

    //    const i32 num_polygons = fbx_mesh->GetPolygonCount();
    //    if(num_polygons <= 0) return false;

    //    // get vertices (in FBX SDK vertices are called Control Points for some reason...
    //    const i32 num_vertices = fbx_mesh->GetControlPointsCount();
    //    FbxVector4* vertices = fbx_mesh->GetControlPoints();
    //    const i32 num_indices = fbx_mesh->GetPolygonVertexCount();
    //    i32* indices = fbx_mesh->GetPolygonVertices();

    //    // if somehow we didn't get any vertices and indices
    //    assert(num_vertices > 0 && vertices && num_indices > 0 && indices);
    //    if(!(num_vertices > 0 && vertices && num_indices > 0 && indices)) return false;

    //    m.raw_indices.resize(num_indices);
    //    
    //    // unify references so that we end up with unique vertices
    //    utl::vector vertex_ref(num_vertices, u32_invalid_id); // utl::vector<u32> where there's num_vertices elements initialized to u32_invalid_id
    //    for (i32 i = 0; i < num_indices; ++i) {
    //        const u32 v_id = (u32)indices[i];
    //        // did we encounter this vertex before? If so, just add its index.
    //        // if not, add vertex and new index.
    //        if (vertex_ref[v_id] != u32_invalid_id) {
    //            m.raw_indices[i] = vertex_ref[v_id];
    //        }
    //        else {
    //            FbxVector4 v = transform.MultT(vertices[v_id]) * _scene_scale; // remember we need to change scale for our engine's side to not bother anymore for no reason.
    //            // filling like so, because in first iteration first index is always 0, then we scale it up and up for positions until we write
    //            // all vertices from FbxMesh into m.positions
    //            m.raw_indices[i] = (u32)m.positions.size();
    //            vertex_ref[v_id] = m.raw_indices[i];
    //            m.positions.emplace_back((f32)v[0], (f32)v[1], (f32)v[2]);
    //        }
    //    }

    //    // make sure we're for real dealing with triangulated mesh (sanity check)
    //    assert(m.raw_indices.size() % 3 == 0);

    //    // get material index per polygon
    //    assert(num_polygons > 0);
    //    // For FbxLayerElementArrayTemplate: https://help.autodesk.com/view/FBX/2020/ENU/?guid=FBX_Developer_Help_cpp_ref_class_fbx_layer_element_array_template_html
    //    FbxLayerElementArrayTemplate<i32>* mtl_indices;
    //    if (fbx_mesh->GetMaterialIndices(&mtl_indices)) { // documentation for this function is quite funny to not say non-existent 
    //                                                                  // (just mentions method's signature
    //        for (i32 i = 0; i < num_polygons; ++i) {
    //            const i32 mtl_index = mtl_indices->GetAt(i);
    //            assert(mtl_index >= 0);
    //            m.material_indices.emplace_back((u32)mtl_index);
    //            // for each polygon do linear search in the material_used array to see if index is already there and if not, we'll add it.
    //            // NOTE: This is not the optimal way to do it, we'll have to change that later
    //            //       though number of materials typically used on a Mesh isn't big, so that might not hurt.
    //            if (std::find(m.material_used.begin(), m.material_used.end(), (u32)mtl_index) == m.material_used.end()) {
    //                m.material_used.emplace_back((u32)mtl_index);
    //            }
    //        }
    //    } // if fbx_mesh->GetMaterialIndices

    //    // Importing normals is ON by default.
    //    const bool import_normals = !_scene_data->settings.calculate_normals;
    //    // Importing tangents is OFF by default.
    //    const bool import_tangents = !_scene_data->settings.calculate_tangents;


    //    // import normals
    //    // NOTE: we don't unify normals, since vertex can have multiple normals depending on which triangle uses it.
    //    if (import_normals) {
    //        FbxArray<FbxVector4> normals;
    //        // calculate normals using FBX's built-in method, but only if normal data isn't there.
    //        if (fbx_mesh->GenerateNormals() && 
    //                fbx_mesh->GetPolygonVertexNormals(normals) && 
    //                normals.Size() > 0) {
    //            // we prefer importing normals over calculating them ourselves, because imported normals can contain edge information
    //            // i.e. which edges need to be hard edges and which soft.
    //            const i32 num_normals = normals.Size();
    //            for (i32 i = 0; i < num_normals; ++i) {
    //                FbxVector4 n{ inverse_transpose.MultT(normals[i]) };
    //                n.Normalize();
    //                m.normals.emplace_back((f32)n[0], (f32)n[1], (f32)n[2]);
    //            }
    //        }
    //        else {
    //            // something went wrong with importing normals from FBX.
    //            // calculate normals on our own.
    //            _scene_data->settings.calculate_normals = true;
    //        }
    //    }


    //    // import tangents
    //    if (import_tangents) {
    //        FbxLayerElementArrayTemplate<FbxVector4>* tangents = nullptr;
    //        // calculate tangents using FBX's built-in method, but only if there's no tangent data already.
    //        if (fbx_mesh->GenerateTangentsData() &&
    //                fbx_mesh->GetTangents(&tangents) &&
    //                tangents && tangents->GetCount() > 0) {
    //            const i32 num_tangents = tangents->GetCount();
    //            for (i32 i = 0; i < num_tangents; ++i) {
    //                FbxVector4 t = tangents->GetAt(i);
    //                // NOTE: tangent values have handedness (contained in 4th component)
    //                const f32 handedness = (f32)t[3];
    //                t[3] = 0.0;
    //                t.Normalize();
    //                // TODO: not sure if this transformation is correct.
    //                t = inverse_transpose.MultT(t);

    //                m.tangents.emplace_back((f32)t[0], (f32)t[1], (f32)t[2], handedness);
    //            }
    //        }
    //        else {
    //            // something went wrong with importing tangents from FBX.
    //            // calculate tangents on our own.
    //            _scene_data->settings.calculate_tangents = true;
    //        }
    //    }

    //    // get UVs
    //    FbxStringList uv_names;
    //    fbx_mesh->GetUVSetNames(uv_names);
    //    const i32 uv_set_count = uv_names.GetCount();
    //    // NOTE: it's OK if we don't have a uv set. E.g. some emissive objects don't need a UV map.
    //    m.uv_sets.resize(uv_set_count);

    //    for (i32 i = 0; i < uv_set_count; ++i) {
    //        FbxArray<FbxVector2> uvs;
    //        if (fbx_mesh->GetPolygonVertexUVs(uv_names.GetStringAt(i), uvs)) {
    //            const i32 num_uvs = uvs.Size();
    //            for (i32 j = 0; j < num_uvs; ++j) {
    //                m.uv_sets[i].emplace_back((f32)uvs[j][0], (f32)uvs[j][1]);
    //            }
    //        }
    //    }

    //    return true;
    //} // bool fbx_context::get_mesh_data()
    





int32_t parse_lod_index(const std::string& node_name, int32_t current_lod) {
    std::smatch match;
    std::regex lod_regex(R"(LOD([0-9]+))", std::regex_constants::icase);
    if (std::regex_search(node_name, match, lod_regex)) {
        return std::stoi(match[1].str());
    }
    return current_lod;
}

void process_node(
    const aiScene* ai_scene,
    const aiNode* node,
    scene& out_scene,
    geometry_import_settings& settings,
    int32_t current_lod,
    bool inside_lod_group = false,
    aiMatrix4x4 parent_transform = aiMatrix4x4() // Pass accumulated transform
) {
    std::string node_name = node->mName.C_Str();

    // Accumulate node transforms down the hierarchy
    aiMatrix4x4 world_transform = parent_transform * node->mTransformation;

    bool is_lod_group_node = (node_name.find("LODGroup") != std::string::npos ||
                              node_name.find("LOD_Group") != std::string::npos ||
                              node_name.find("LodGroup") != std::string::npos);

    if (!inside_lod_group) {
        int32_t detected_lod = parse_lod_index(node_name, -1);
        if (detected_lod != -1) {
            current_lod = detected_lod;
        }
    }

    // Pre-calculate 3x3 rotation matrix for normals/tangents
    aiMatrix3x3 rotation_matrix(world_transform);

    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        unsigned int mesh_idx = node->mMeshes[i];
        const aiMesh* ai_mesh = ai_scene->mMeshes[mesh_idx];

        if (!ai_mesh->HasPositions() || ai_mesh->mNumVertices == 0) continue;

        mesh m{};
        m.name = ai_mesh->mName.length > 0 ? ai_mesh->mName.C_Str() : node_name;
        m.lod_id = current_lod;
        m.lod_treshhold = -1.0f;

        // Positions: Transform by node world matrix into unified Y-up space
        m.positions.reserve(ai_mesh->mNumVertices);
        for (unsigned int v = 0; v < ai_mesh->mNumVertices; ++v) {
            aiVector3D pos = world_transform * ai_mesh->mVertices[v];
            m.positions.push_back({ pos.x, pos.y, pos.z });
        }

        // Indices
        const u32 num_indices = ai_mesh->mNumFaces * 3;
        m.raw_indices.reserve(num_indices);
        for (unsigned int f = 0; f < ai_mesh->mNumFaces; ++f) {
            const aiFace& face = ai_mesh->mFaces[f];
            if (face.mNumIndices == 3) {
                m.raw_indices.push_back(face.mIndices[0]);
                m.raw_indices.push_back(face.mIndices[1]);
                m.raw_indices.push_back(face.mIndices[2]);
            }
        }

        if (m.raw_indices.empty()) continue;

        // Normals: Transform by node rotation matrix
        const bool import_normals = !settings.calculate_normals;
        if (import_normals && ai_mesh->HasNormals()) {
            m.normals.reserve(num_indices);
            for (unsigned int f = 0; f < ai_mesh->mNumFaces; ++f) {
                const aiFace& face = ai_mesh->mFaces[f];
                for (unsigned int idx = 0; idx < face.mNumIndices; ++idx) {
                    u32 v_idx = face.mIndices[idx];
                    aiVector3D n = rotation_matrix * ai_mesh->mNormals[v_idx];
                    m.normals.push_back({ n.x, n.y, n.z });
                }
            }
        }
        else {
            settings.calculate_normals = true;
        }

        // Tangents: Transform by node rotation matrix
        const bool import_tangents = !settings.calculate_tangents;
        if (import_tangents && ai_mesh->HasTangentsAndBitangents()) {
            m.tangents.reserve(num_indices);
            for (unsigned int f = 0; f < ai_mesh->mNumFaces; ++f) {
                const aiFace& face = ai_mesh->mFaces[f];
                for (unsigned int idx = 0; idx < face.mNumIndices; ++idx) {
                    u32 v_idx = face.mIndices[idx];
                    aiVector3D t = rotation_matrix * ai_mesh->mTangents[v_idx];
                    aiVector3D b = rotation_matrix * ai_mesh->mBitangents[v_idx];
                    aiVector3D n = rotation_matrix * ai_mesh->mNormals[v_idx];

                    float handedness = ((n ^ t) * b < 0.0f) ? -1.0f : 1.0f;
                    m.tangents.push_back({ t.x, t.y, t.z, handedness });
                }
            }
        }
        else {
            settings.calculate_tangents = true;
        }

        // UV Sets
        unsigned int num_uv_channels = 0;
        while (num_uv_channels < AI_MAX_NUMBER_OF_TEXTURECOORDS && ai_mesh->HasTextureCoords(num_uv_channels)) {
            num_uv_channels++;
        }

        m.uv_sets.resize(num_uv_channels);
        for (unsigned int ch = 0; ch < num_uv_channels; ++ch) {
            m.uv_sets[ch].reserve(num_indices);
            for (unsigned int f = 0; f < ai_mesh->mNumFaces; ++f) {
                const aiFace& face = ai_mesh->mFaces[f];
                for (unsigned int idx = 0; idx < face.mNumIndices; ++idx) {
                    u32 v_idx = face.mIndices[idx];
                    aiVector3D uv = ai_mesh->mTextureCoords[ch][v_idx];
                    m.uv_sets[ch].push_back({ uv.x, uv.y });
                }
            }
        }

        if (out_scene.lod_groups.size() <= static_cast<size_t>(current_lod)) {
            out_scene.lod_groups.resize(current_lod + 1);
        }
        if (out_scene.lod_groups[current_lod].name.empty()) {
            out_scene.lod_groups[current_lod].name = "LOD_" + std::to_string(current_lod);
        }

        out_scene.lod_groups[current_lod].meshes.push_back(std::move(m));
    }

    // Recurse children with world_transform passed down
    for (unsigned int c = 0; c < node->mNumChildren; ++c) {
        int32_t next_lod = current_lod;
        bool next_inside_lod_group = inside_lod_group;

        if (is_lod_group_node) {
            next_lod = static_cast<int32_t>(c);
            next_inside_lod_group = true;
        }

        process_node(ai_scene, node->mChildren[c], out_scene, settings, next_lod, next_inside_lod_group, world_transform);
    }
}

void consolidate_lod_meshes(scene& out_scene) {
    for (auto& lod_group : out_scene.lod_groups) {
        if (lod_group.meshes.size() <= 1) continue;

        mesh combined_mesh{};
        combined_mesh.name = lod_group.name;
        combined_mesh.lod_id = lod_group.meshes[0].lod_id;
        combined_mesh.lod_treshhold = lod_group.meshes[0].lod_treshhold;

        u32 vertex_offset = 0;

        for (const auto& sub_mesh : lod_group.meshes) {
            // Combine positions
            combined_mesh.positions.insert(
                combined_mesh.positions.end(),
                sub_mesh.positions.begin(),
                sub_mesh.positions.end()
            );

            // Combine indices with proper offset calculation
            for (u32 idx : sub_mesh.raw_indices) {
                combined_mesh.raw_indices.push_back(idx + vertex_offset);
            }

            // Combine normals
            combined_mesh.normals.insert(
                combined_mesh.normals.end(),
                sub_mesh.normals.begin(),
                sub_mesh.normals.end()
            );

            // Combine tangents
            combined_mesh.tangents.insert(
                combined_mesh.tangents.end(),
                sub_mesh.tangents.begin(),
                sub_mesh.tangents.end()
            );

            // Combine UV channels
            if (combined_mesh.uv_sets.size() < sub_mesh.uv_sets.size()) {
                combined_mesh.uv_sets.resize(sub_mesh.uv_sets.size());
            }
            for (size_t ch = 0; ch < sub_mesh.uv_sets.size(); ++ch) {
                combined_mesh.uv_sets[ch].insert(
                    combined_mesh.uv_sets[ch].end(),
                    sub_mesh.uv_sets[ch].begin(),
                    sub_mesh.uv_sets[ch].end()
                );
            }

            vertex_offset += static_cast<u32>(sub_mesh.positions.size());
        }

        // Replace all sub-mesh pieces with the consolidated mesh
        lod_group.meshes.clear();
        lod_group.meshes.push_back(std::move(combined_mesh));
    }
}

void consolidate_all_lods_into_single_asset(scene& out_scene) {
    if (out_scene.lod_groups.size() <= 1) return;

    // Preserve LOD 0 as the single container for all LOD levels
    lod_group unified_group = std::move(out_scene.lod_groups[0]);

    for (size_t i = 1; i < out_scene.lod_groups.size(); ++i) {
        for (auto& m : out_scene.lod_groups[i].meshes) {
            unified_group.meshes.push_back(std::move(m));
        }
    }

    out_scene.lod_groups.clear();
    out_scene.lod_groups.push_back(std::move(unified_group));
}

void import_fbx(const char* file_path, scene& out_scene, geometry_import_settings& settings) {
    Assimp::Importer importer;

    /*const aiScene* ai_scene = importer.ReadFile(file_path,
                                                aiProcess_Triangulate |
                                                aiProcess_JoinIdenticalVertices |
                                                aiProcess_SortByPType |
                                                aiProcess_GenSmoothNormals |
                                                aiProcess_CalcTangentSpace |
                                                aiProcess_GlobalScale
    );*/
    const aiScene* ai_scene = importer.ReadFile(file_path,
                                                aiProcess_Triangulate |
                                                aiProcess_JoinIdenticalVertices |
                                                aiProcess_SortByPType |
                                                aiProcess_GenSmoothNormals |
                                                aiProcess_CalcTangentSpace |
                                                aiProcess_GlobalScale |
                                                aiProcess_ConvertToLeftHanded // Normalizes coordinate space and axis orientation
    );

    if (!ai_scene || !ai_scene->mRootNode) return;

    process_node(ai_scene, ai_scene->mRootNode, out_scene, settings, 0, false);

    // 1. Merge parented sub-meshes within each LOD level (reduces 12 pieces -> 4 LOD meshes)
    consolidate_lod_meshes(out_scene);


    // 2. Combine all 4 LOD levels into 1 single scene entity so exporter saves exactly 1 asset file
    consolidate_all_lods_into_single_asset(out_scene);

    out_scene.generate_default_lod_thresholds();
    // out_scene.generate_default_lod_thresholds();
}


MAGE_ED_INTERFACE void ImportFbx(const char* file, scene_data* data) {
    assert(file && data);
    scene scene{};

    // NOTE: anything that involves using the FBX SDK should be single-threaded
    {
        // got to lock since editor will try to import objects in parallel
        std::lock_guard lock{fbx_mutex};
        import_fbx(file, scene, data->settings);
        // ctor initializes and loads fbx file
        //fbx_context context{file, &scene, data};
        //if (context.is_valid()) {
        //    context.get_scene();
        //}
        //else {
        //    // TODO: send failure log message to editor
        //    return;
        //}
    }

    // engine-side processing
    process_scene(scene, data->settings);
    pack_data(scene, *data);
} // MAGE_ED_INTERFACE void ImportFbx()


} // namespace mage::tools