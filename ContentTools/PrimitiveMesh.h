#pragma once

#include "ToolsCommon.h"
#include "Geometry.h"

namespace mage::tools {
    enum primitive_mesh_types : u32 {
        plane = 0u,
        cube,
        uv_sphere,
        ico_sphere,
        cylinder,
        capsule,

        count
    };

    struct primitive_mesh_init_info {
        // what type of primitive do we want to use
        primitive_mesh_types    type;
        u32                     segments[3]{ 1,1,1 }; // how we want primitive to be subdivided
        math::vec3              size{ 1,1,1 }; // initial size of mesh that we generate
        u32                     lod{ 0 };
    };
}