#pragma once
#include "D3D12CommonHeaders.h"

namespace mage::gfx::d3d12::core {
    bool initialize();
    void shutdown();

    void render();

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

    ID3D12Device* const device();
    u32 current_frame_index();

    /// <summary>
    ///  Indicates that resources for this frame have to be released.
    /// <para/>
    /// It's not locked, because writes to integers in x86 architecture are atomic anyway.
    /// </summary>
    void set_deferred_releases_flag();
}