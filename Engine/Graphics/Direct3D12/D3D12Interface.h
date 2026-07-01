#pragma once

namespace mage::gfx {
    struct platform_interface;

    namespace d3d12 {
        void get_platform_interface(platform_interface &pi);
    }

} // namespace mage::gfx