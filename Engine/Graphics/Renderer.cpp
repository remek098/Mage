#include "Renderer.h"
#include "GfxPlatformInterface.h"
#include "Direct3D12/D3D12Interface.h"

namespace mage::gfx {
    namespace {
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

        return true;
    }

    bool initialize(gfx_platform platform) {
        return set_platform_interface(platform) && gfx_interface.initialize();
    }

    void shutdown() {
        gfx_interface.shutdown();
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
} // namespace mage::gfx