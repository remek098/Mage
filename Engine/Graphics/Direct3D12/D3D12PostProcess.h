#pragma once
#include "D3D12CommonHeaders.h"

namespace mage::gfx::d3d12 {
struct d3d12_frame_info;
}

namespace mage::gfx::d3d12::fx {

    bool initialize();
    void shutdown();


    void post_process(id3d12_graphics_command_list* cmd_list, 
                      const d3d12_frame_info& info, 
                      D3D12_CPU_DESCRIPTOR_HANDLE target_rtv);
}