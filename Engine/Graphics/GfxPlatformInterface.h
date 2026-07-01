#pragma once
#include "CommonHeaders.h"
#include "Renderer.h"

namespace mage::gfx {

    /// <summary>
    /// struct with function pointers that we need to implement in low level renderer
    /// avoiding polymorphism, because we will have 1 renderer with 1 specific rendering API
    /// </summary>
    struct platform_interface {
        bool (*initialize)(void);
        void (*shutdown)(void);
        void (*render)(void);
    };
}