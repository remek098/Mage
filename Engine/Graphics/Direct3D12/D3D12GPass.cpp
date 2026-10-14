#include "D3D12GPass.h"
#include "D3D12Shaders.h"
#include "D3D12Content.h"
#include "D3D12Camera.h"
#include "Shaders/ShaderTypes.h"
#include "Components/Entity.h"
#include "Components/Transform.h"

namespace mage::gfx::d3d12::gpass {
namespace {


constexpr math::u32vec2         initial_dimensions{ 100, 100 };

d3d12_render_texture            gpass_main_buffer{};
d3d12_depth_buffer              gpass_depth_buffer{};
math::u32vec2                   dimensions{ initial_dimensions }; // remember the size of buffers since they might get resized.


#if _DEBUG
constexpr f32                   clear_value[4]{ 0.5f, 0.5, 0.5f, 1.f }; // lighter gray color in debug build

#else
constexpr f32                   clear_value[4]{}; // black color to clear render target

#endif


#if USE_STL_VECTOR
#define CONSTEXPR
#else
#define CONSTEXPR constexpr
#endif

struct gpass_cache {
    utl::vector<id::id_type>    d3d12_render_item_ids;

    // NOTE: when adding new arrays, make sure to update resize() and struct_size.
    id::id_type*                entity_ids{ nullptr };
    id::id_type*                submesh_gpu_ids{ nullptr };
    id::id_type*                material_ids{ nullptr };
    ID3D12PipelineState**       gpass_pipeline_states{ nullptr };
    ID3D12PipelineState**       depth_pipeline_states{ nullptr };
    ID3D12RootSignature**       root_signatures{ nullptr };
    material_type::type*        material_types{ nullptr };
    D3D12_GPU_VIRTUAL_ADDRESS*  position_buffers{ nullptr };
    D3D12_GPU_VIRTUAL_ADDRESS*  element_buffers{ nullptr };
    D3D12_INDEX_BUFFER_VIEW*    index_buffer_views{ nullptr };
    D3D_PRIMITIVE_TOPOLOGY*     primitive_topologies{ nullptr };
    u32*                        elements_types{ nullptr };
    D3D12_GPU_VIRTUAL_ADDRESS*  per_object_data{ nullptr };

    constexpr content::render_item::items_cache items_cache() const {
        return {
            entity_ids,
            submesh_gpu_ids,
            material_ids,
            gpass_pipeline_states,
            depth_pipeline_states
        };
    }

    constexpr content::submesh::views_cache views_cache() const {
        return {
            position_buffers,
            element_buffers,
            index_buffer_views,
            primitive_topologies,
            elements_types
        };
    }

    constexpr content::material::materials_cache materials_cache() const {
        return {
            root_signatures,
            material_types
        };
    }

    CONSTEXPR u32 size() const {
        return (u32)d3d12_render_item_ids.size();
    }

    CONSTEXPR void clear() {
        return d3d12_render_item_ids.clear();
    }

    CONSTEXPR void resize() {
        const u64 items_count{ d3d12_render_item_ids.size() };
        const u64 new_buffer_size{ items_count * struct_size };
        const u64 old_buffer_size{ _buffer.size() };
        if (new_buffer_size > old_buffer_size) {
            _buffer.resize(new_buffer_size);
        }

        // set data pointers to point to the offsets within resized buffer.
        if (new_buffer_size != old_buffer_size) {
            entity_ids = (id::id_type*)_buffer.data();  
            submesh_gpu_ids = (id::id_type*)(&entity_ids[items_count]);
            material_ids = (id::id_type*)(&submesh_gpu_ids[items_count]);
            gpass_pipeline_states = (ID3D12PipelineState**)(&material_ids[items_count]);
            depth_pipeline_states = (ID3D12PipelineState**)(&gpass_pipeline_states[items_count]);
            root_signatures = (ID3D12RootSignature**)(&depth_pipeline_states[items_count]);
            material_types = (material_type::type*)(&root_signatures[items_count]);
            position_buffers = (D3D12_GPU_VIRTUAL_ADDRESS*)(&material_types[items_count]);
            element_buffers = (D3D12_GPU_VIRTUAL_ADDRESS*)(&position_buffers[items_count]);
            index_buffer_views = (D3D12_INDEX_BUFFER_VIEW*)(&element_buffers[items_count]);
            primitive_topologies = (D3D_PRIMITIVE_TOPOLOGY*)(&index_buffer_views[items_count]);
            elements_types = (u32*)(&primitive_topologies[items_count]);
            per_object_data = (D3D12_GPU_VIRTUAL_ADDRESS*)(&elements_types[items_count]);
        }
    }

private:
    constexpr static u32 struct_size{
        sizeof(id::id_type) +                       // entity_ids
        sizeof(id::id_type) +                       // submesh_ids
        sizeof(id::id_type) +                       // material_ids
        sizeof(ID3D12PipelineState*) +              // gpass_pipeline_states
        sizeof(ID3D12PipelineState*) +              // depth_pipeline_states
        sizeof(ID3D12RootSignature*) +              // root_signatures
        sizeof(material_type::type) +               // material_types
        sizeof(D3D12_GPU_VIRTUAL_ADDRESS) +         // position_buffers
        sizeof(D3D12_GPU_VIRTUAL_ADDRESS) +         // element_buffers
        sizeof(D3D12_INDEX_BUFFER_VIEW) +           // index_buffer_views
        sizeof(D3D_PRIMITIVE_TOPOLOGY) +            // primitive_topologies
        sizeof(u32) +                               // elements_types
        sizeof(D3D12_GPU_VIRTUAL_ADDRESS)           // per_object_data
    };
    
    utl::vector<u8>     _buffer;
} frame_cache;

bool create_buffers(math::u32vec2 size) {
    assert(size.x && size.y);
    gpass_main_buffer.release(); // because this function will also be called when buffers are being resized.
    gpass_depth_buffer.release();

    D3D12_RESOURCE_DESC desc{};
    desc.Alignment = 0; // NOTE: 0 is the same as 64KB (or 4MB for MSAA multisampled textures)
    desc.DepthOrArraySize = 1;
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    desc.Format = main_buffer_format;
    desc.Height = size.y;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // driver chooses the optimal layout
    desc.MipLevels = 0; // make space for all mip levels
    desc.SampleDesc = { 1, 0 };
    desc.Width = size.x;

    // create main buffer
    {
        d3d12_texture_init_info info{};
        info.desc = &desc;
        // initial state is PIXEL_SHADER_RESOURCE 
        // and we will alternate between render target and resource states using resource transitions.
        info.initial_state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        info.clear_value.Format = desc.Format;
        memcpy(&info.clear_value.Color, &clear_value[0], sizeof(clear_value));
        gpass_main_buffer = d3d12_render_texture{ info };
    }

    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    desc.Format = depth_buffer_format;
    desc.MipLevels = 1; // we only need 1 mip level for depth buffer.
            
    // create the depth buffer
    {
        d3d12_texture_init_info info{};
        info.desc = &desc;
        // will alternate between D3D12_RESOURCE_STATE_DEPTH_READ and D3D12_RESOURCE_STATE_DEPTH_WRITE
        info.initial_state =    D3D12_RESOURCE_STATE_DEPTH_READ |
                                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
                                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE; // for possibility to read it in compute shader.
        info.clear_value.Format = desc.Format;
        info.clear_value.DepthStencil.Depth = 0.f; // reversed depth
        info.clear_value.DepthStencil.Stencil = 0;

        gpass_depth_buffer = d3d12_depth_buffer{ info };
    }

    NAME_D3D12_OBJECT(gpass_main_buffer.resource(), L"GPass Main Buffer");
    NAME_D3D12_OBJECT(gpass_depth_buffer.resource(), L"GPass Depth Buffer");

    // return whether both resources are valid.
    return gpass_main_buffer.resource() && gpass_depth_buffer.resource();
} // bool create_buffers()


void
fill_per_object_data(const d3d12_frame_info& info) {
    const gpass_cache& cache{ frame_cache };

    const u32 render_items_count{ cache.size() };
    // NOTE: more than one item could belong to the same geometry -> therefore to the same game_entity,
    // we can use same PerObjectData for groups of render items from the same game_entity.
    // This way we can save the space in constant_buffer that would otherwise be wasted.
    // As long as current_entity_id matches the entity_id inside cache, we can use the same PerObjectData pointer in the constant buffer.
    id::id_type current_entity_id{ id::invalid_id };
    hlsl::PerObjectData* current_data_ptr{ nullptr };

    constant_buffer& cbuffer{ core::cbuffer() };

    using namespace DirectX;
    for (u32 i{ 0 }; i < render_items_count; ++i) {
        if (current_entity_id != cache.entity_ids[i]) {
            current_entity_id = cache.entity_ids[i];
            hlsl::PerObjectData data{};
            transform::get_transform_matrices(game_entity::entity_id{ current_entity_id }, data.World, data.InvWorld);
            XMMATRIX world{ XMLoadFloat4x4(&data.World) };
            XMMATRIX world_view_projection{ XMMatrixMultiply(world, info.camera->view_projection()) };
            XMStoreFloat4x4(&data.WorldViewProjection, world_view_projection);

            current_data_ptr = cbuffer.allocate<hlsl::PerObjectData>();
            memcpy(current_data_ptr, &data, sizeof(hlsl::PerObjectData));
        }

        assert(current_data_ptr);
        cache.per_object_data[i] = cbuffer.gpu_address(current_data_ptr);
    }
}

void
set_root_parameters(id3d12_graphics_command_list* const cmd_list, u32 cache_index) {
    gpass_cache& cache{ frame_cache };
    assert(cache_index < cache.size());

    const material_type::type mat_type{ cache.material_types[cache_index] };
    switch (mat_type) {
        case material_type::opaque:
        {
            using params = opaque_root_parameter;
            cmd_list->SetGraphicsRootShaderResourceView(params::position_buffer, cache.position_buffers[cache_index]);
            cmd_list->SetGraphicsRootShaderResourceView(params::element_buffer, cache.element_buffers[cache_index]);
            cmd_list->SetGraphicsRootConstantBufferView(params::per_object_data, cache.per_object_data[cache_index]);
        } break;
    }
}

/// <summary>
/// populates global gpass frame cache with data.
/// </summary>
/// <param name="info"></param>
void
prepare_render_frame(const d3d12_frame_info& info) {
    assert(info.info && info.camera);
    assert(info.info->render_item_ids && info.info->render_item_count);
    gpass_cache& cache{ frame_cache };
    cache.clear();

    using namespace content;
    render_item::get_d3d12_render_item_ids(*info.info, cache.d3d12_render_item_ids);
    cache.resize();
    const u32 items_count{ cache.size() };

    const render_item::items_cache items_cache{ cache.items_cache() };
    render_item::get_items(cache.d3d12_render_item_ids.data(), items_count, items_cache);

    const submesh::views_cache views_cache{ cache.views_cache() };
    submesh::get_views(items_cache.submesh_gpu_ids, items_count, views_cache);

    const material::materials_cache materials_cache{ cache.materials_cache() };
    material::get_materials(items_cache.material_ids, items_count, materials_cache);

    constant_buffer& cbuffer{ core::cbuffer() }; // cbuffer for current frame
    fill_per_object_data(info);
}
#undef CONSTEXPR
} // anonymous namespace

bool initialize() {
    return create_buffers(initial_dimensions);
}

void shutdown() {
    gpass_main_buffer.release();
    gpass_depth_buffer.release();
    dimensions = initial_dimensions;

}

const d3d12_render_texture& main_buffer() {
    return gpass_main_buffer;
}

const d3d12_depth_buffer& depth_buffer() {
    return gpass_depth_buffer;
}

void set_size(math::u32vec2 size) {
    math::u32vec2& d{ dimensions };
    if (size.x > d.x || size.y > d.y) {
        d = { std::max(size.x, d.x), std::max(size.y, d.y) };
        create_buffers(d);
    }
}
void depth_prepass(id3d12_graphics_command_list* cmd_list, const d3d12_frame_info& info) {
    prepare_render_frame(info);


    const gpass_cache& cache{ frame_cache };
    const u32 items_count{ cache.size() };

    ID3D12RootSignature* current_root_signature{ nullptr };
    ID3D12PipelineState* current_pipeline_state{ nullptr };

    for (u32 i{ 0 }; i < items_count; ++i) {
        if (current_root_signature != cache.root_signatures[i]) {
            current_root_signature = cache.root_signatures[i];
            cmd_list->SetGraphicsRootSignature(current_root_signature);
            // This root parameter doesn't change for each render item and we need to set it once when we set a diffrent root signature.
            cmd_list->SetGraphicsRootConstantBufferView(opaque_root_parameter::global_shader_data, info.global_shader_data);
        }

        if (current_pipeline_state != cache.depth_pipeline_states[i]) {
            current_pipeline_state = cache.depth_pipeline_states[i];
            cmd_list->SetPipelineState(current_pipeline_state);
        }

        set_root_parameters(cmd_list, i); // those that change per render item.

        const D3D12_INDEX_BUFFER_VIEW& ibv{ cache.index_buffer_views[i] };
        const u32 index_count{ ibv.SizeInBytes >> (ibv.Format == DXGI_FORMAT_R16_UINT ? 1 : 2) }; // diffrent index count depending on index format.
        cmd_list->IASetIndexBuffer(&ibv);
        cmd_list->IASetPrimitiveTopology(cache.primitive_topologies[i]);
        cmd_list->DrawIndexedInstanced(index_count, 1, 0, 0, 0);
    }
}

void render(id3d12_graphics_command_list* cmd_list, const d3d12_frame_info& info) {
    const gpass_cache& cache{ frame_cache };
    const u32 items_count{ cache.size() };

    ID3D12RootSignature* current_root_signature{ nullptr };
    ID3D12PipelineState* current_pipeline_state{ nullptr };

    for (u32 i{ 0 }; i < items_count; ++i) {
        if (current_root_signature != cache.root_signatures[i]) {
            current_root_signature = cache.root_signatures[i];
            cmd_list->SetGraphicsRootSignature(current_root_signature);
            // This root parameter doesn't change for each render item and we need to set it once when we set a diffrent root signature.
            cmd_list->SetGraphicsRootConstantBufferView(opaque_root_parameter::global_shader_data, info.global_shader_data);
        }

        if (current_pipeline_state != cache.gpass_pipeline_states[i]) {
            current_pipeline_state = cache.gpass_pipeline_states[i];
            cmd_list->SetPipelineState(current_pipeline_state);
        }

        set_root_parameters(cmd_list, i); // those that change per render item.

        const D3D12_INDEX_BUFFER_VIEW& ibv{ cache.index_buffer_views[i] };
        const u32 index_count{ ibv.SizeInBytes >> (ibv.Format == DXGI_FORMAT_R16_UINT ? 1 : 2) }; // diffrent index count depending on index format.
        cmd_list->IASetIndexBuffer(&ibv);
        cmd_list->IASetPrimitiveTopology(cache.primitive_topologies[i]);
        cmd_list->DrawIndexedInstanced(index_count, 1, 0, 0, 0);
    }
}


void add_transitions_for_depth_prepass(d3dx::d3d12_resource_barrier& barriers) {
    // initial state was D3D12_RESOURCE_STATE_DEPTH_READ |
    //                   D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
    //                   D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    barriers.add(gpass_main_buffer.resource(),
                    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                    D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY);
    barriers.add(gpass_depth_buffer.resource(),
                    D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                    D3D12_RESOURCE_STATE_DEPTH_WRITE);
}
    
void add_transitions_for_gpass(d3dx::d3d12_resource_barrier& barriers) {
    barriers.add(gpass_main_buffer.resource(),
                    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                    D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_BARRIER_FLAG_END_ONLY);
    barriers.add(gpass_depth_buffer.resource(),
                    D3D12_RESOURCE_STATE_DEPTH_WRITE,
                    D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}
    
void add_transitions_for_post_process(d3dx::d3d12_resource_barrier& barriers) {
    barriers.add(gpass_main_buffer.resource(),
                    D3D12_RESOURCE_STATE_RENDER_TARGET,
                    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}


void set_render_targets_for_depth_prepass(id3d12_graphics_command_list* cmd_list) {
    const D3D12_CPU_DESCRIPTOR_HANDLE dsv{ gpass_depth_buffer.dsv() };
    cmd_list->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 0.f, 0, 0, nullptr);
    cmd_list->OMSetRenderTargets(0, nullptr, 0, &dsv);
}

void set_render_targets_for_gpass(id3d12_graphics_command_list* cmd_list) {
    const D3D12_CPU_DESCRIPTOR_HANDLE rtv{ gpass_main_buffer.rtv(0) }; // rtv for first mip level of the buffer
    const D3D12_CPU_DESCRIPTOR_HANDLE dsv{ gpass_depth_buffer.dsv() };

    cmd_list->ClearRenderTargetView(rtv, clear_value, 0, nullptr);
    cmd_list->OMSetRenderTargets(1, &rtv, 0, &dsv);
}
} // namespace mage::gfx::d3d12::gpass