#pragma once

#include "ToolsCommon.h"

namespace mage::tools {
//namespace packed_vertex {
//    struct static_vertex {
//        math::vec3      position;
//        u8              reserved[3];
//        // bit 0: tangent handedness * (tangent.z sign), bit 1: normal.z sign (0 means -1, 1 means +1)
//        u8              t_sign;
//        // simple trick, in shader we can calculate z coordinate of vertex, but we need to know direction of normal vector
//        // to choose wheter z is positive or negative sign -> bit 1 of t_sign
//        u16             normal[2];
//        u16             tangent[2];
//        math::vec2      uv;
//    };
//} // namespace packed vertex


struct vertex {
    math::vec4      tangent{};
    math::vec4      joint_weights{};
    math::u32vec4   joint_indices{ u32_invalid_id, u32_invalid_id, u32_invalid_id, u32_invalid_id };
    math::vec3      position{};
    math::vec3      normal{};
    math::vec2      uv{};

    u8              red{}, green{}, blue{};
    u8              pad;
};

namespace vertex_elements {
// NOTE: all sizes of corresponding structs need to be multiple of 4 bytes
struct elements_type {
    enum type : u32 {
        position_only                   = 0x00,
        static_normal                   = 0x01,
        static_normal_texture           = 0x03, // normal and texture bits set to 1
        static_color                    = 0x04,
        skeletal                        = 0x08,
        skeletal_color                  = skeletal | static_color,
        skeletal_normal                 = skeletal | static_normal,
        skeletal_normal_color           = skeletal_normal | static_color,
        skeletal_normal_texture         = skeletal | static_normal_texture,
        skeletal_normal_texture_color   = skeletal_normal_texture | static_color,
    };
};

// below until end of namespace are structs for each of combinations of vertex elements.

// 4 bytes size, 1 byte aligned struct
struct static_color {
    u8      color[3];
    u8      pad;
};

// 8 bytes size, 2 byte aligned struct
struct static_normal {
    u8      color[3];
    u8      t_sign; // bit 0: tangent handedness * (tangent.z sign), bit 1: normal.z sign (0 means -1; 1 means +1)
    u16     normal[2]; // shader will calculate 3rd normal
};

// 20 bytes size, 4 byte aligned struct
struct static_normal_texture {
    u8              color[3];
    u8              t_sign; // bit 0: tangent handedness * (tangent.z sign), bit 1: normal.z sign (0 means -1; 1 means +1)
    u16             normal[2]; // shader will calculate 3rd normal
    u16             tangent[2];
    math::vec2      uv;
};

// 12 bytes size, 2 byte aligned struct
struct skeletal {
    u8      joint_weights[3]; // normalized joint weight for up to 4 joints.
    u8      pad;
    u16     joint_indices[4];
};

// 16 bytes size, 2 byte aligned struct
struct skeletal_color {
    u8      joint_weights[3]; // normalized joint weight for up to 4 joints.
    u8      pad;
    u16     joint_indices[4];
    u8      color[3];
    u8      pad2;
};

// 16 byte size, 2 byte aligned
struct skeletal_normal {
    u8      joint_weights[3]; // normalized joint weight for up to 4 joints.
    u8      t_sign; // bit 0: tangent handedness * (tangent.z sign), bit 1: normal.z sign (0 means -1; 1 means +1)
    u16     joint_indices[4];
    u16     normal[2]; // shader will calculate 3rd normal
};

// 20 byte size, 2 byte aligned
struct skeletal_normal_color {
    u8      joint_weights[3]; // normalized joint weight for up to 4 joints.
    u8      t_sign; // bit 0: tangent handedness * (tangent.z sign), bit 1: normal.z sign (0 means -1; 1 means +1)
    u16     joint_indices[4];
    u16     normal[2]; // shader will calculate 3rd normal
    u8      color[3];
    u8      pad;
};

// 16 byte size, 4 byte aligned
struct skeletal_normal_texture {
    u8              joint_weights[3]; // normalized joint weight for up to 4 joints.
    u8              t_sign; // bit 0: tangent handedness * (tangent.z sign), bit 1: normal.z sign (0 means -1; 1 means +1)
    u16             joint_indices[4];
    u16             normal[2]; // shader will calculate 3rd normal
    u16             tangent[2];
    math::vec2      uv;
};

// 32 byte size, 4 byte aligned
struct skeletal_normal_texture_color {
    u8              joint_weights[3]; // normalized joint weight for up to 4 joints.
    u8              t_sign; // bit 0: tangent handedness * (tangent.z sign), bit 1: normal.z sign (0 means -1; 1 means +1)
    u16             joint_indices[4];
    u16             normal[2]; // shader will calculate 3rd normal
    u16             tangent[2];
    math::vec2      uv;
    u8              color[3];
    u8              pad;
};

} // namespace vertex_elements

struct mesh {
    utl::vector<math::vec3>                 positions;
    utl::vector<math::vec3>                 normals;
    utl::vector<math::vec4>                 tangents;
    utl::vector<math::vec3>                 colors;
    utl::vector<utl::vector<math::vec2>>    uv_sets; // we might have diffrent uv_sets for a mesh
    utl::vector<u32>                        material_indices; // per polygon material indices
    utl::vector<u32>                        material_used;  // id of each material used in this mesh

    utl::vector<u32>                        raw_indices;

    // intermediate data
    utl::vector<vertex>                     vertices;
    utl::vector<u32>                        indices;
        
    // output data -> result of vertex processing
    std::string                             name;
    vertex_elements::elements_type::type    elements_type;
    utl::vector<u8>                         position_buffer;
    // untyped element raw buffer that we can cast depending on what elements_type is.
    utl::vector<u8>                         element_buffer;
    f32                                     lod_treshold{ -1.f }; // lod_treshold telling when to switch to another LOD
    u32                                     lod_id{ u32_invalid_id }; // denotes which meshes belong together in one LOD object
};

struct parsed_lod_name {
    std::string base_name;
    u32 lod_id = 0;
    bool is_lod = false;
};

struct lod_group {
    std::string         name;
    utl::vector<mesh>   meshes;
};

struct scene {
    std::string             name;
    utl::vector<lod_group>  lod_groups; // group of meshes, that represent that object and all level of details for that object

    void add_mesh(mesh m);
    void generate_default_lod_thresholds();
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

    

parsed_lod_name parse_lod_name(std::string_view name);
void process_scene(scene& scene, const geometry_import_settings& settings);
void pack_data(const scene& scene, scene_data& data);
}