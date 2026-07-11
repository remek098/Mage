#pragma once
#include "CommonHeaders.h"
#include "Renderer.h"
#include "Platform\Window.h"

namespace mage::gfx {

    /// <summary>
    /// struct with function pointers that we need to implement in low level renderer
    /// avoiding polymorphism, because we will have 1 renderer with 1 specific rendering API
    /// </summary>
    struct platform_interface {
        bool (*initialize)(void);
        void (*shutdown)(void);

        struct {
            surface(*create)(platform::window);
            void(*remove)(surface_id);
            void(*resize)(surface_id, u32, u32);
            u32(*width)(surface_id);
            u32(*height)(surface_id);
            void(*render)(surface_id);
        } surface;

        gfx_platform platform = (gfx_platform)-1;
    };
}