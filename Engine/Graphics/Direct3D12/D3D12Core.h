#pragma once
#include "D3D12CommonHeaders.h"

// forward declarations
namespace mage::gfx::d3d12 {
    class descriptor_heap;
}

namespace mage::gfx::d3d12::core {

    bool initialize();
    void shutdown();


    template<typename T>
    constexpr void release(T*& resource) {
        if (resource) {
            resource->Release();
            resource = nullptr;
        }
    }

    namespace detail {
        void deferred_release(IUnknown* resource);
    }

    template<typename T>
    constexpr void deferred_release(T*& resource) {
        if (resource) {
            detail::deferred_release(resource);
            resource = nullptr;
        }
    }

    ID3D12Device* const get_device();
    u32 get_current_frame_index();


    descriptor_heap& rtv_heap();
    descriptor_heap& dsv_heap();
    descriptor_heap& srv_heap();
    descriptor_heap& uav_heap();

    /// <summary>
    /// returns DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
    /// </summary>
    /// <returns></returns>
    DXGI_FORMAT default_render_target_format();

    /// <summary>
    ///  Indicates that resources for this frame have to be released.
    /// <para/>
    /// It's not locked, because writes to integers in x86 architecture are atomic anyway.
    /// </summary>
    void set_deferred_releases_flag();


    surface create_surface(platform::window window);
    void remove_surface(surface_id id);
    void resize_surface(surface_id id, u32 width, u32 height);
    u32 surface_width(surface_id id);
    u32 surface_height(surface_id id);
    void render_surface(surface_id id);
}