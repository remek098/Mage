#pragma once

#include "CommonHeaders.h"

namespace mage::platform {
    DEFINE_TYPED_ID(window_id)

    /// <summary>
    /// Abstraction for a platform-specific windows handling
    /// </summary>
    class window {
    public:
        constexpr explicit window(window_id id) : _id{ id } {}
        constexpr window() : _id{ id::invalid_id } {}

        constexpr window_id get_id() { return _id; }
        constexpr bool is_valid() const { return id::is_valid(_id); }

        void set_fullscreen(bool is_fullscreen) const;
        bool is_fullscreen() const;
        
        void* handle() const;
        void set_caption(const wchar_t* caption) const;
        math::u32vec4 size() const;
        void resize(u32 width, u32 height) const;
        u32 width() const;
        u32 height() const;
        bool is_closed() const;

    private:
        // can have multiple windows to e.g. have multiple windows in editor to e.g. have model viewer, main renderer view that shows game scene
        window_id _id{ id::invalid_id };
    };
}