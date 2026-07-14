#pragma once
#include "D3D12Core.h"

namespace mage::gfx::d3d12 {
    struct d3d12_frame_info;
}

namespace mage::gfx::d3d12::gpass {

    bool initialize();
    void shutdown();


    [[nodiscard]] const d3d12_render_texture& main_buffer();
    [[nodiscard]] const d3d12_depth_buffer& depth_buffer();


    // NOTE: call this every frame before rendering anything to gpass.
    void set_size(math::u32vec2 size);
    // depth prepass will give depth information for light culling and lighting calculation.
    void depth_prepass(id3d12_graphics_command_list* cmd_list, const d3d12_frame_info& info);
    void render(id3d12_graphics_command_list* cmd_list, const d3d12_frame_info& info);

    void add_transitions_for_depth_prepass(d3dx::d3d12_resource_barrier& barriers);
    void add_transitions_for_gpass(d3dx::d3d12_resource_barrier& barriers);
    void add_transitions_for_post_process(d3dx::d3d12_resource_barrier& barriers);

    void set_render_targets_for_depth_prepass(id3d12_graphics_command_list* cmd_list);
    void set_render_targets_for_gpass(id3d12_graphics_command_list* cmd_list);
}
