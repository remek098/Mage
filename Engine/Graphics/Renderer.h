#pragma once
#include "..\Platform\Window.h"
#include "CommonHeaders.h"

namespace mage::gfx {

    class surface {};

    struct render_surface {
        platform::window window{};
        surface surface{};
    };

    enum class gfx_platform : u32 {
        d3d12 = 0,
    };

    bool initialize(gfx_platform platform);
    void shutdown();

    void render();
} // namespace mage::gfx