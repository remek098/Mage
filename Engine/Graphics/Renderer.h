#pragma once
#include "CommonHeaders.h"
#include "Platform/Window.h"
#include "EngineAPI/Camera.h"

namespace mage::gfx {
DEFINE_TYPED_ID(surface_id)
    
class surface {
public:
    constexpr explicit surface(surface_id id) : _id{ id } {}
    constexpr surface() = default;

    constexpr surface_id get_id() { return _id; }
    constexpr bool is_valid() const { return id::is_valid(_id); }

        
    void resize(u32 width, u32 height) const;
    u32 width() const;
    u32 height() const;
    void render() const;

private:
    // can have multiple windows to e.g. have multiple windows in editor to e.g. have model viewer, main renderer view that shows game scene
    surface_id _id{ id::invalid_id };
};

struct render_surface {
    platform::window window{};
    surface surface{};
};

struct camera_parameter {
    enum parameter : u32 {
        up_vector,
        field_of_view,
        aspect_ratio,
        view_width,
        view_height,
        near_z,
        far_z,
        view,
        projection,
        inverse_projection,
        view_projection,
        inverse_view_projection,
        type,
        entity_id,

        count
    };
};

struct camera_init_info {
    id::id_type         entity_id{ id::invalid_id };
    camera::type        type{};
    math::vec3          up;

    union {
        f32 field_of_view; // perspective camera
        f32 view_width;    // ortographic camera
    };
    union {
        f32 aspect_ratio; // perspective camera
        f32 view_height;  // ortographic camera
    };

    f32 near_z;
    f32 far_z;
};

struct perspective_camera_init_info : public camera_init_info {
    explicit perspective_camera_init_info(id::id_type id) {
        assert(id::is_valid(id));
        entity_id = id;
        type = camera::type::perspective;
        up = { 0.f, 1.f, 0.f }; // +Y axis
        field_of_view = 0.25f; // 45 degrees.
        aspect_ratio = 16.f / 10.f;
        near_z = 0.001f; // 1 mm
        far_z = 10000.f; // 10 km
    }
};

struct ortographic_camera_init_info : public camera_init_info {
    explicit ortographic_camera_init_info(id::id_type id) {
        assert(id::is_valid(id));
        entity_id = id;
        type = camera::type::ortographic;
        up = { 0.f, 1.f, 0.f }; // +Y axis
        view_width = 1920;
        view_height = 1080;
        near_z = 0.001f;
        far_z = 10000.f;
    }
};

enum class gfx_platform : u32 {
    d3d12 = 0,
};

bool initialize(gfx_platform platform);
void shutdown();

// Get the location of compiled engine shaders relative to exe's path.
// The path is for the graphics API that is currently in use.
const char* get_engine_shaders_path();
// Get the location of compiled engine shaders, for the specified platform, relative to exe's path.
// The path is for the graphics API that is currently in use.
const char* get_engine_shaders_path(gfx_platform platform);


surface create_surface(platform::window window);
void remove_surface(surface_id id);

camera create_camera(camera_init_info info);
void  remove_camera(camera_id id);

id::id_type add_submesh(const u8*& data);
void remove_submesh(id::id_type id);
} // namespace mage::gfx