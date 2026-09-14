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

        struct {
            camera(*create)(camera_init_info);
            void(*remove)(camera_id);

            // void* is data, u32 is sizeof that data
            void(*set_parameter)(camera_id, camera_parameter::parameter, const void* const, u32);
            void(*get_parameter)(camera_id, camera_parameter::parameter, void* const, u32);
        } camera;

        struct {
            id::id_type (*add_submesh)(const u8*&);
            void (*remove_submesh)(id::id_type);
        } resources;

        gfx_platform platform = (gfx_platform)-1;
    };
}