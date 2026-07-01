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
} // namespace mage::gfx