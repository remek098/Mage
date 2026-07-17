#pragma once
#include "..\Platform\Window.h"
#include "CommonHeaders.h"

namespace mage::gfx {
    DEFINE_TYPED_ID(surface_id)
    
    class surface {
    public:
        constexpr explicit surface(surface_id id) : _id{ id } {}
        constexpr surface() = default;

        constexpr surface_id get_id() { return _id; }
        constexpr bool is_valid() const { return id::is_valid(_id); }

        
        void resize(u32 width, u32 height) const;
        u32 width() const;
        u32 height() const;
        void render() const;

    private:
        // can have multiple windows to e.g. have multiple windows in editor to e.g. have model viewer, main renderer view that shows game scene
        surface_id _id{ id::invalid_id };
    };

    struct render_surface {
        platform::window window{};
        surface surface{};
    };

    enum class gfx_platform : u32 {
        d3d12 = 0,
    };

    bool initialize(gfx_platform platform);
    void shutdown();

    // Get the location of compiled engine shaders relative to exe's path.
    // The path is for the graphics API that is currently in use.
    const char* get_engine_shaders_path();
    // Get the location of compiled engine shaders, for the specified platform, relative to exe's path.
    // The path is for the graphics API that is currently in use.
    const char* get_engine_shaders_path(gfx_platform platform);


    surface create_surface(platform::window window);
    void remove_surface(surface_id id);

} // namespace mage::gfx