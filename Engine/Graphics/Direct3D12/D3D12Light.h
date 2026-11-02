#pragma once

#include "D3D12CommonHeaders.h"

namespace mage::gfx::d3d12 {
struct d3d12_frame_info;
}

namespace mage::gfx::d3d12::light {

bool initialize();
void shutdown();

/// <summary>
/// Adds new light to the specified light_set in light_init_info parameter. (std::unordered_map<u64, light_set> underneath holds all light_sets)
/// </summary>
/// <param name="info">Struct that tells to what light_set what type of light and with what parameters to be created and added.</param>
/// <returns>gfx::light which basically contains light_id for the added light and light_set_key so that user can reference this light.</returns>
gfx::light create(light_init_info info);

/// <summary>
/// Removes the light from the specified light_set. (std::unordered_map<u64, light_set> underneath)
/// </summary>
/// <param name="id">light_id pointing to the entity containing the light.</param>
/// <param name="light_set_key">The key to specify from which light_set should the light be removed.</param>
void remove(light_id id, u64 light_set_key);

/// <summary>
/// sets appropriate parameter of light. Please note that data and size must be appropriate to set parameter.
/// </summary>
/// <param name="id"></param>
/// <param name="light_set_key"></param>
/// <param name="parameter">Tells which parameter to set.</param>
/// <param name="data">Pointer to the data that is going to be set for the specified light.</param>
/// <param name="size">Size of the parameter to be set. Needs to match the size of sent data <para/>
/// (which needs to match the size of the type you want to set.)</param>
void set_parameter(light_id id, u64 light_set_key, light_parameter::parameter parameter, const void *const data, u32 size);

/// <summary>
/// Gets appropriate parameter of light. Please note that data and size must be appropriate to parameter you want to get.
/// </summary>
/// <param name="id"></param>
/// <param name="light_set_key"></param>
/// <param name="parameter">Tells which parameter to get.</param>
/// <param name="data">Pointer to the data that retrieves parameter you want to get.</param>
/// <param name="size">Size of the parameter you want to retrieve. Needs to match the size of sent data <para/>
/// (which needs to match the size of the type you want to set.)</param>
void get_parameter(light_id id, u64 light_set_key, light_parameter::parameter parameter, void *const data, u32 size);

void update_light_buffers(const d3d12_frame_info& d3d12_info);
D3D12_GPU_VIRTUAL_ADDRESS non_cullable_light_buffer(u32 frame_index);
u32 non_cullable_light_count(u64 light_set_key);

}