#pragma once

#include "ToolsCommon.h"

namespace mage::tools {
    namespace packed_vertex {
        struct static_vertex {
            math::vec3      position;
            u8              reserved[3];
            // bit 0: tangent handedness * (tangent.z sign), bit 1: normal.z sign (0 means -1, 1 means +1)
            u8              t_sign;
            // simple trick, in shader we can calculate z coordinate of vertex, but we need to know direction of normal vector
            // to choose wheter z is positive or negative sign -> bit 1 of t_sign
            u16             normal[2];
            u16             tangent[2];
            math::vec2      uv;
        };
    } // namespace packed vertex


    struct vertex {
        math::vec4 tangent{};
        math::vec3 position{};
        math::vec3 normal{};
        math::vec2 uv{};
    };

    struct mesh {
        utl::vector<math::vec3>                 positions;
        utl::vector<math::vec3>                 normals;
        utl::vector<math::vec4>                 tangents;
        utl::vector<utl::vector<math::vec2>>    uv_sets; // we might have diffrent uv_sets for a mesh

        utl::vector<u32>                        raw_indices;

        // intermediate data
        utl::vector<vertex>                     vertices;
        utl::vector<u32>                        indices;
        
        // output data -> result of vertex processing
        std::string                                 name;
        utl::vector<packed_vertex::static_vertex>   packed_static_vertices; // packed vertices for use in shader
        f32                                         lod_treshhold{ -1.f }; // lod_treshhold telling when to switch to another LOD
        u32                                         lod_id{ u32_invalid_id }; // denotes which meshes belong together in one LOD object
    };

    struct lod_group {
        std::string         name;
        utl::vector<mesh>   meshes;
    };

    struct scene {
        std::string             name;
        utl::vector<lod_group>  lod_groups; // group of meshes, that represent that object and all level of details for that object
    };

    struct geometry_import_settings {
        f32 smoothing_angle;
        u8  calculate_normals; // we can tell importer, to calculate normals instead of using imported once
        u8  calculate_tangents; // like normals, but parallel to the plane defined by triangle
        u8  reverse_handedness; // mage uses right-handed coordinate system; imported meshes might need to be converted from left_handed
        u8  import_embeded_textures;
        u8  import_animations;
    };

    struct scene_data {
        u8*                         buffer;
        u32                         buffer_size;
        geometry_import_settings    settings;
    };

    


    void process_scene(scene& scene, const geometry_import_settings& settings);
    void pack_data(const scene& scene, scene_data& data);
}