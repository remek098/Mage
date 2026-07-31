#include "FbxImporter.h"
#include "Geometry.h"

namespace mage::tools {
    namespace {
        std::mutex fbx_mutex{};
    } // anonymous namespace

    bool fbx_context::initialize_fbx() {
        // https://help.autodesk.com/view/FBX/2020/ENU/?guid=FBX_Developer_Help_getting_started_your_first_fbx_sdk_program_html
        assert(!is_valid());
        _fbx_manager = FbxManager::Create();
        if(!_fbx_manager) return false;

        // https://help.autodesk.com/view/FBX/2020/ENU/?guid=FBX_Developer_Help_importing_and_exporting_a_scene_io_settings_html
        FbxIOSettings* ios = FbxIOSettings::Create(_fbx_manager, IOSROOT);
        _fbx_manager->SetIOSettings(ios);

        return true;
    }

    void fbx_context::load_fbx_file(const char* file) {
        assert(_fbx_manager && !_fbx_scene);
        _fbx_scene = FbxScene::Create(_fbx_manager, "Importer Scene");
        if(!_fbx_scene) return;

        FbxImporter* importer = FbxImporter::Create(_fbx_manager, "Importer");
        if (!(importer &&
              importer->Initialize(file, -1, _fbx_manager->GetIOSettings()) && 
              importer->Import(_fbx_scene))) {
            // on failure to create, initialize or import
            if(importer) importer->Destroy();
            return;
        }
        importer->Destroy();

        // get scene scale in meters.
        _scene_scale = (f32)_fbx_scene->GetGlobalSettings().GetSystemUnit().GetConversionFactorTo(FbxSystemUnit::m);
    }


    

    void fbx_context::get_scene(FbxNode* root /* = nullptr */) {
        assert(is_valid());
        // https://help.autodesk.com/view/FBX/2020/ENU/?guid=FBX_Developer_Help_nodes_and_scene_graph_fbx_scenes_html
        // https://help.autodesk.com/view/FBX/2020/ENU/?guid=FBX_Developer_Help_nodes_and_scene_graph_fbx_nodes_html
        if (!root) {
            root = _fbx_scene->GetRootNode();
            if (!root) return;
        }
        get_meshes(root);

        // fill in defaults only for meshes that don't already have tresholds.
        _scene->generate_default_lod_thresholds();
    }

    void fbx_context::get_mesh(
                               FbxNodeAttribute* attribute,
                               const std::string& group_name,
                               u32 lod_id,
                               f32 lod_threshold) {
        assert(attribute);

        FbxMesh* fbx_mesh = static_cast<FbxMesh*>(attribute);
        if (fbx_mesh->RemoveBadPolygons() < 0) return;

        FbxGeometryConverter gc{ _fbx_manager };
        fbx_mesh = static_cast<FbxMesh*>(gc.Triangulate(fbx_mesh, true));
        if (!fbx_mesh || fbx_mesh->RemoveBadPolygons() < 0) return;

        mesh m{};
        m.name = group_name;
        m.lod_id = lod_id;
        m.lod_treshhold = lod_threshold;

        if (!get_mesh_data(fbx_mesh, m)) return;

        _scene->add_mesh(std::move(m));
    }

    void fbx_context::get_meshes(
                                 FbxNode* node,
                                 const std::string& group_name,
                                 u32 lod_id,
                                 f32 lod_threshold) {
        assert(node);

        const i32 attribute_count = node->GetNodeAttributeCount();

        for (i32 i = 0; i < attribute_count; ++i) {
            FbxNodeAttribute* attribute =
                node->GetNodeAttributeByIndex(i);

            if (!attribute)
                continue;

            if (attribute->GetAttributeType() ==
                FbxNodeAttribute::eMesh) {
                get_mesh(attribute,
                         group_name,
                         lod_id,
                         lod_threshold);
            }
        }

        const i32 child_count = node->GetChildCount();

        for (i32 i = 0; i < child_count; ++i) {
            get_meshes(node->GetChild(i),
                       group_name,
                       lod_id,
                       lod_threshold);
        }
    }


    void fbx_context::get_meshes(FbxNode* node) {
        assert(node);

        const i32 attribute_count = node->GetNodeAttributeCount();
        for (i32 i = 0; i < attribute_count; ++i) {
            FbxNodeAttribute* attribute = node->GetNodeAttributeByIndex(i);

            if (!attribute) continue;

            switch (attribute->GetAttributeType()) {
                case FbxNodeAttribute::eMesh:
                    get_mesh(attribute);
                    break;

                case FbxNodeAttribute::eLODGroup:
                    get_lod_group(attribute);
                    return;
            }
        }

        const i32 child_count = node->GetChildCount();
        for (i32 i = 0; i < child_count; ++i) {
            get_meshes(node->GetChild(i));
        }
    }

    void fbx_context::get_mesh(FbxNodeAttribute* attribute) {
        assert(attribute);

        FbxMesh* fbx_mesh = static_cast<FbxMesh*>(attribute);
        if (fbx_mesh->RemoveBadPolygons() < 0) return;

        FbxGeometryConverter gc{ _fbx_manager };
        fbx_mesh = static_cast<FbxMesh*>(gc.Triangulate(fbx_mesh, true));
        if (!fbx_mesh || fbx_mesh->RemoveBadPolygons() < 0) return;

        mesh m{};
        FbxNode* node = fbx_mesh->GetNode();

        m.name = node->GetName()[0] ?
            node->GetName() :
            fbx_mesh->GetName();

        if (!get_mesh_data(fbx_mesh, m)) return;

        auto info = parse_lod_name(m.name);
        if (info.is_lod) {
            m.name = info.base_name;
            m.lod_id = info.lod_id;
        }
        else {
            m.lod_id = 0;
        }

        _scene->add_mesh(std::move(m));
    }



    void fbx_context::get_lod_group(FbxNodeAttribute* attribute) {
        assert(attribute);
        // https://help.autodesk.com/view/FBX/2015/ENU/?guid=__cpp_ref_class_fbx_l_o_d_group_html -> for some reason it's referenced in FBX 2015 doc, but not for 2020
        auto* lod_grp = static_cast<FbxLODGroup*>(attribute);
        FbxNode* node = lod_grp->GetNode();

        const std::string group_name = node->GetName()[0] ? 
            node->GetName() :
            lod_grp->GetName();

        // NOTE: number of LODs is exclusive to the base mesh (LOD0)
        const i32 child_count = node->GetChildCount();

        assert(child_count > 0);

        for (i32 i = 0; i < child_count; ++i) {
            f32 lod_threshold = -1.f;

            if (i > 0) {
                FbxDistance distance;
                lod_grp->GetThreshold(i - 1, distance);

                lod_threshold =
                    distance.value() * _scene_scale;
            }

            get_meshes(node->GetChild(i), group_name, (u32)i, lod_threshold);
        }
    }

    bool fbx_context::get_mesh_data(FbxMesh* fbx_mesh, mesh& m) {
        assert(fbx_mesh);

        FbxNode* const node = fbx_mesh->GetNode();
        FbxAMatrix geo_transform_mat;

        geo_transform_mat.SetT(node->GetGeometricTranslation(FbxNode::eSourcePivot));
        geo_transform_mat.SetR(node->GetGeometricRotation(FbxNode::eSourcePivot));
        geo_transform_mat.SetS(node->GetGeometricScaling(FbxNode::eSourcePivot));

        FbxAMatrix transform{node->EvaluateGlobalTransform() * geo_transform_mat};
        FbxAMatrix inverse_transpose{transform.Inverse().Transpose()};

        const i32 num_polygons = fbx_mesh->GetPolygonCount();
        if(num_polygons <= 0) return false;

        // get vertices (in FBX SDK vertices are called Control Points for some reason...
        const i32 num_vertices = fbx_mesh->GetControlPointsCount();
        FbxVector4* vertices = fbx_mesh->GetControlPoints();
        const i32 num_indices = fbx_mesh->GetPolygonVertexCount();
        i32* indices = fbx_mesh->GetPolygonVertices();

        // if somehow we didn't get any vertices and indices
        assert(num_vertices > 0 && vertices && num_indices > 0 && indices);
        if(!(num_vertices > 0 && vertices && num_indices > 0 && indices)) return false;

        m.raw_indices.resize(num_indices);
        
        // unify references so that we end up with unique vertices
        utl::vector vertex_ref(num_vertices, u32_invalid_id); // utl::vector<u32> where there's num_vertices elements initialized to u32_invalid_id
        for (i32 i = 0; i < num_indices; ++i) {
            const u32 v_id = (u32)indices[i];
            // did we encounter this vertex before? If so, just add its index.
            // if not, add vertex and new index.
            if (vertex_ref[v_id] != u32_invalid_id) {
                m.raw_indices[i] = vertex_ref[v_id];
            }
            else {
                FbxVector4 v = transform.MultT(vertices[v_id]) * _scene_scale; // remember we need to change scale for our engine's side to not bother anymore for no reason.
                // filling like so, because in first iteration first index is always 0, then we scale it up and up for positions until we write
                // all vertices from FbxMesh into m.positions
                m.raw_indices[i] = (u32)m.positions.size();
                vertex_ref[v_id] = m.raw_indices[i];
                m.positions.emplace_back((f32)v[0], (f32)v[1], (f32)v[2]);
            }
        }

        // make sure we're for real dealing with triangulated mesh (sanity check)
        assert(m.raw_indices.size() % 3 == 0);

        // get material index per polygon
        assert(num_polygons > 0);
        // For FbxLayerElementArrayTemplate: https://help.autodesk.com/view/FBX/2020/ENU/?guid=FBX_Developer_Help_cpp_ref_class_fbx_layer_element_array_template_html
        FbxLayerElementArrayTemplate<i32>* mtl_indices;
        if (fbx_mesh->GetMaterialIndices(&mtl_indices)) { // documentation for this function is quite funny to not say non-existent 
                                                                      // (just mentions method's signature
            for (i32 i = 0; i < num_polygons; ++i) {
                const i32 mtl_index = mtl_indices->GetAt(i);
                assert(mtl_index >= 0);
                m.material_indices.emplace_back((u32)mtl_index);
                // for each polygon do linear search in the material_used array to see if index is already there and if not, we'll add it.
                // NOTE: This is not the optimal way to do it, we'll have to change that later
                //       though number of materials typically used on a Mesh isn't big, so that might not hurt.
                if (std::find(m.material_used.begin(), m.material_used.end(), (u32)mtl_index) == m.material_used.end()) {
                    m.material_used.emplace_back((u32)mtl_index);
                }
            }
        } // if fbx_mesh->GetMaterialIndices

        // Importing normals is ON by default.
        const bool import_normals = !_scene_data->settings.calculate_normals;
        // Importing tangents is OFF by default.
        const bool import_tangents = !_scene_data->settings.calculate_tangents;


        // import normals
        // NOTE: we don't unify normals, since vertex can have multiple normals depending on which triangle uses it.
        if (import_normals) {
            FbxArray<FbxVector4> normals;
            // calculate normals using FBX's built-in method, but only if normal data isn't there.
            if (fbx_mesh->GenerateNormals() && 
                    fbx_mesh->GetPolygonVertexNormals(normals) && 
                    normals.Size() > 0) {
                // we prefer importing normals over calculating them ourselves, because imported normals can contain edge information
                // i.e. which edges need to be hard edges and which soft.
                const i32 num_normals = normals.Size();
                for (i32 i = 0; i < num_normals; ++i) {
                    FbxVector4 n{ inverse_transpose.MultT(normals[i]) };
                    n.Normalize();
                    m.normals.emplace_back((f32)n[0], (f32)n[1], (f32)n[2]);
                }
            }
            else {
                // something went wrong with importing normals from FBX.
                // calculate normals on our own.
                _scene_data->settings.calculate_normals = true;
            }
        }


        // import tangents
        if (import_tangents) {
            FbxLayerElementArrayTemplate<FbxVector4>* tangents = nullptr;
            // calculate tangents using FBX's built-in method, but only if there's no tangent data already.
            if (fbx_mesh->GenerateTangentsData() &&
                    fbx_mesh->GetTangents(&tangents) &&
                    tangents && tangents->GetCount() > 0) {
                const i32 num_tangents = tangents->GetCount();
                for (i32 i = 0; i < num_tangents; ++i) {
                    FbxVector4 t = tangents->GetAt(i);
                    // NOTE: tangent values have handedness (contained in 4th component)
                    const f32 handedness = (f32)t[3];
                    t[3] = 0.0;
                    t.Normalize();
                    // TODO: not sure if this transformation is correct.
                    t = inverse_transpose.MultT(t);

                    m.tangents.emplace_back((f32)t[0], (f32)t[1], (f32)t[2], handedness);
                }
            }
            else {
                // something went wrong with importing tangents from FBX.
                // calculate tangents on our own.
                _scene_data->settings.calculate_tangents = true;
            }
        }

        // get UVs
        FbxStringList uv_names;
        fbx_mesh->GetUVSetNames(uv_names);
        const i32 uv_set_count = uv_names.GetCount();
        // NOTE: it's OK if we don't have a uv set. E.g. some emissive objects don't need a UV map.
        m.uv_sets.resize(uv_set_count);

        for (i32 i = 0; i < uv_set_count; ++i) {
            FbxArray<FbxVector2> uvs;
            if (fbx_mesh->GetPolygonVertexUVs(uv_names.GetStringAt(i), uvs)) {
                const i32 num_uvs = uvs.Size();
                for (i32 j = 0; j < num_uvs; ++j) {
                    m.uv_sets[i].emplace_back((f32)uvs[j][0], (f32)uvs[j][1]);
                }
            }
        }

        return true;
    } // bool fbx_context::get_mesh_data()

    


    MAGE_ED_INTERFACE void ImportFbx(const char* file, scene_data* data) {
        assert(file && data);
        scene scene{};

        // NOTE: anything that involves using the FBX SDK should be single-threaded
        {
            // got to lock since editor will try to import objects in parallel
            std::lock_guard lock{fbx_mutex};
            // ctor initializes and loads fbx file
            fbx_context context{file, &scene, data};
            if (context.is_valid()) {
                context.get_scene();
            }
            else {
                // TODO: send failure log message to editor
                return;
            }
        }

        // engine-side processing
        process_scene(scene, data->settings);
        pack_data(scene, *data);
    } // MAGE_ED_INTERFACE void ImportFbx()


} // namespace mage::tools