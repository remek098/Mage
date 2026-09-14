#include "Renderer.h"
#include "GfxPlatformInterface.h"
#include "Direct3D12/D3D12Interface.h"

namespace mage::gfx {
namespace {
    // Defines where the compiled engine shaders file is located for each one of the supported graphics API.
    constexpr const char* engine_shader_paths[]{
        ".\\shaders\\d3d12\\shaders.bin",
        // ".\\shaders\\vulkan\\shaders.bin", etc if you ever wanted to add other graphics API in future
    };

    platform_interface gfx_interface{};
} // anonymous namespace

bool set_platform_interface(gfx_platform platform) {
    switch (platform) {
    case gfx_platform::d3d12:
        d3d12::get_platform_interface(gfx_interface);
        break;
    default:
        return false;
    }

    assert(gfx_interface.platform == platform);
    return true;
}

bool initialize(gfx_platform platform) {
    return set_platform_interface(platform) && gfx_interface.initialize();
}

void shutdown() {
    // unnecessary if application shuts down only if initialization succedded
    if (gfx_interface.platform != (gfx_platform)-1) gfx_interface.shutdown();
}


const char* get_engine_shaders_path() {
    return engine_shader_paths[(u32)gfx_interface.platform];
}

const char* get_engine_shaders_path(gfx_platform platform) {
    return engine_shader_paths[(u32)platform];
}

surface create_surface(platform::window window) {
    return gfx_interface.surface.create(window);
}

void remove_surface(surface_id id) {
    assert(id::is_valid(id));
    gfx_interface.surface.remove(id);
}
void surface::resize(u32 width, u32 height) const {
    assert(id::is_valid(_id));
    gfx_interface.surface.resize(_id, width, height);
}
u32 surface::width() const {
    assert(id::is_valid(_id));
    return gfx_interface.surface.width(_id);
}
u32 surface::height() const {
    assert(id::is_valid(_id));
    return gfx_interface.surface.height(_id);
}
void surface::render() const {
    assert(id::is_valid(_id));
    gfx_interface.surface.render(_id);
}

camera 
create_camera(camera_init_info info) {
    return gfx_interface.camera.create(info);
}

void remove_camera(camera_id id) {
    gfx_interface.camera.remove(id);
}

void 
camera::up(math::vec3 up) const {
    assert(is_valid());
    gfx_interface.camera.set_parameter(_id, camera_parameter::up_vector, &up, sizeof(up));
}

void 
camera::field_of_view(f32 fov) const {
    assert(is_valid());
    gfx_interface.camera.set_parameter(_id, camera_parameter::field_of_view, &fov, sizeof(fov));
}

void 
camera::aspect_ratio(f32 aspect_ratio) const {
    assert(is_valid());
    gfx_interface.camera.set_parameter(_id, camera_parameter::aspect_ratio, &aspect_ratio, sizeof(aspect_ratio));
}

void 
camera::view_width(f32 width) const {
    assert(is_valid());
    gfx_interface.camera.set_parameter(_id, camera_parameter::view_width, &width, sizeof(width));
}

void 
camera::view_height(f32 height) const {
    assert(is_valid());
    gfx_interface.camera.set_parameter(_id, camera_parameter::view_height, &height, sizeof(height));
}

void 
camera::range(f32 near_z, f32 far_z) const {
    assert(is_valid());
    gfx_interface.camera.set_parameter(_id, camera_parameter::near_z, &near_z, sizeof(near_z));
    gfx_interface.camera.set_parameter(_id, camera_parameter::far_z, &far_z, sizeof(far_z));
}

math::mat4x4 
camera::view() const {
    assert(is_valid());
    math::mat4x4 matrix;
    gfx_interface.camera.get_parameter(_id, camera_parameter::view, &matrix, sizeof(matrix));
    return matrix;
}

math::mat4x4 
camera::projection() const {
    assert(is_valid());
    math::mat4x4 matrix;
    gfx_interface.camera.get_parameter(_id, camera_parameter::projection, &matrix, sizeof(matrix));
    return matrix;
}

math::mat4x4 
camera::inverse_projection() const {
    assert(is_valid());
    math::mat4x4 matrix;
    gfx_interface.camera.get_parameter(_id, camera_parameter::inverse_projection, &matrix, sizeof(matrix));
    return matrix;
}

math::mat4x4 
camera::view_projection() const {
    assert(is_valid());
    math::mat4x4 matrix;
    gfx_interface.camera.get_parameter(_id, camera_parameter::view_projection, &matrix, sizeof(matrix));
    return matrix;
}

math::mat4x4 
camera::inverse_view_projection() const {
    assert(is_valid());
    math::mat4x4 matrix;
    gfx_interface.camera.get_parameter(_id, camera_parameter::inverse_view_projection, &matrix, sizeof(matrix));
    return matrix;
}


math::vec3 
camera::up() const {
    assert(is_valid());
    math::vec3 up_vec;
    gfx_interface.camera.get_parameter(_id, camera_parameter::up_vector, &up_vec, sizeof(up_vec));
    return up_vec;
}

f32 
camera::near_z() const {
    assert(is_valid());
    f32 near_z;
    gfx_interface.camera.get_parameter(_id, camera_parameter::near_z, &near_z, sizeof(near_z));
    return near_z;
}

f32 
camera::far_z() const {
    assert(is_valid());
    f32 far_z;
    gfx_interface.camera.get_parameter(_id, camera_parameter::far_z, &far_z, sizeof(far_z));
    return far_z;
}

f32 
camera::field_of_view() const {
    assert(is_valid());
    f32 fov;
    gfx_interface.camera.get_parameter(_id, camera_parameter::field_of_view, &fov, sizeof(fov));
    return fov;
}

f32 
camera::aspect_ratio() const {
    assert(is_valid());
    f32 ratio;
    gfx_interface.camera.get_parameter(_id, camera_parameter::aspect_ratio, &ratio, sizeof(ratio));
    return ratio;
}

f32 
camera::view_width() const {
    assert(is_valid());
    f32 width;
    gfx_interface.camera.get_parameter(_id, camera_parameter::view_width, &width, sizeof(width));
    return width;
}

f32 
camera::view_height() const {
    assert(is_valid());
    f32 height;
    gfx_interface.camera.get_parameter(_id, camera_parameter::view_height, &height, sizeof(height));
    return height;
}


camera::type 
camera::projection_type() const {
    assert(is_valid());
    camera::type type;
    gfx_interface.camera.get_parameter(_id, camera_parameter::type, &type, sizeof(type));
    return type;
}

id::id_type 
camera::entity_id() const {
    assert(is_valid());
    id::id_type entity_id;
    gfx_interface.camera.get_parameter(_id, camera_parameter::entity_id, &entity_id, sizeof(entity_id));
    return entity_id;
}

id::id_type 
add_submesh(const u8*& data) {
    return gfx_interface.resources.add_submesh(data);
}

void 
remove_submesh(id::id_type id) {
    gfx_interface.resources.remove_submesh(id);
}

} // namespace mage::gfx