#include "D3D12Interface.h"
#include "CommonHeaders.h"
#include "D3D12Core.h"
#include "Graphics\GfxPlatformInterface.h"

namespace mage::gfx::d3d12 {
    void get_platform_interface(platform_interface& pi) {
        pi.initialize = core::initialize;
        pi.shutdown = core::shutdown;
    }
} // namespace mage::gfx::d3d12
