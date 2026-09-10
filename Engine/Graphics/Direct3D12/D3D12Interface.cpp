#include "D3D12Interface.h"
#include "CommonHeaders.h"
#include "D3D12Core.h"
#include "D3D12Content.h"
#include "Graphics\GfxPlatformInterface.h"

namespace mage::gfx::d3d12 {
    void get_platform_interface(platform_interface& pi) {
        pi.initialize = core::initialize;
        pi.shutdown = core::shutdown;

        pi.surface.create = core::create_surface;
        pi.surface.remove = core::remove_surface;
        pi.surface.resize = core::resize_surface;
        pi.surface.width = core::surface_width;
        pi.surface.height = core::surface_height;
        pi.surface.render = core::render_surface;

        pi.resources.add_submesh = content::submesh::add;
        pi.resources.remove_submesh = content::submesh::remove;

        pi.platform = gfx_platform::d3d12;
    }
} // namespace mage::gfx::d3d12
