#include "D3D12Resources.h"
#include "D3D12Core.h"

namespace mage::gfx::d3d12 {
    /////       DESCRIPTOR HEAP //////////////////////////////////////////////////////////////////////////////////////////////
    bool descriptor_heap::init(u32 capacity, bool is_shader_visible) {
        // this function might and probably will be used on diffrent threads (for textures at the very least)
        std::lock_guard lock{ _mutex };
        // make sure we don't cross d3d12.h defined limits
        // we're going to limit even non-shader-visible descriptor heaps to these values
        assert(capacity && capacity < D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2);
        assert(!(_type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER && capacity > D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE));

        // for RTV and DSV it's not possible to create shader visible descriptor heaps
        if (_type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV || _type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV) {
            is_shader_visible = false;
        }

        release(); // because this function can be used multiple times

        ID3D12Device* const device = core::get_device();
        assert(device);

        D3D12_DESCRIPTOR_HEAP_DESC desc{};
        desc.Flags = is_shader_visible
            ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
            : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NumDescriptors = capacity;
        desc.NodeMask = 0;
        desc.Type = _type;

        HRESULT hr = S_OK;
        DXCALL( hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&_heap)));
        if (FAILED(hr)) return false;

        _free_handles = std::move(std::make_unique<u32[]>(capacity));
        _capacity = capacity;
        _size = 0;

        // every single slot in this heap is available for allocation
        for (u32 i = 0; i < capacity; ++i) _free_handles[i] = i;
        // if we initialize for 2nd time and onwards, _deferred_free_indices might contain some deferred releases.
        DEBUG_ONLY_EXPR(for (u32 i = 0; i < frame_buffer_count; ++i) assert(_deferred_free_indices[i].empty()));


        _descriptor_size = device->GetDescriptorHandleIncrementSize(_type);
        _cpu_start = _heap->GetCPUDescriptorHandleForHeapStart();
        _gpu_start = is_shader_visible ? _heap->GetGPUDescriptorHandleForHeapStart() : D3D12_GPU_DESCRIPTOR_HANDLE{ 0 };

        return true;
    }

    void descriptor_heap::release() {
        assert(!_size);
        core::deferred_release(_heap);
    }

    void descriptor_heap::process_deferred_free(u32 frame_index) {
        std::lock_guard lock{ _mutex };
        assert(frame_index < frame_buffer_count);

        // if there're indices/slots that are needed to be freed in descriptor_heap, we proceed to do so
        utl::vector<u32>& indices = _deferred_free_indices[frame_index];
        if (!indices.empty()) {
            for (auto index : indices) {
                --_size;
                _free_handles[_size] = index; // add that index at the end of free_handles (array that kinda acts similar as utl::vector
                                              // unless someone completely screws up somehow somewhere.

            }
            indices.clear(); // clear array of indices, because well, we processed everything that we had to process.
        }
    }

    descriptor_handle descriptor_heap::allocate() {
        // _mutex is unlocked after lock_guard goes out of scope (in it's destructor)
        std::lock_guard lock{ _mutex };
        assert(_heap);
        assert(_size < _capacity); // if that fails, it would mean we can't allocate more descriptors in here.

        const u32 index = _free_handles[_size];
        const u32 offset = index * _descriptor_size;
        ++_size; // ofc indicate new descriptor has been added

        descriptor_handle handle;
        handle.cpu.ptr = _cpu_start.ptr + offset;
        if (is_shader_visible()) handle.gpu.ptr = _gpu_start.ptr + offset;

        DEBUG_ONLY_EXPR(handle.container = this);
        DEBUG_ONLY_EXPR(handle.index = index);
        return handle;
    }

    void descriptor_heap::free(descriptor_handle& handle) {
        if (!handle.is_valid()) return;
        std::lock_guard lock{ _mutex };
        // assert happens only in debug mode. In release it's stripped out
        // couple of checks to see if the handle is within this heap and if it belongs to this heap at all
        assert(_heap && _size); // heap has to exist and heap cannot be empty, obviously
        assert(handle.container == this); // check if this handle belongs to this heap
        assert(handle.cpu.ptr >= _cpu_start.ptr);
        assert((handle.cpu.ptr - _cpu_start.ptr) % _descriptor_size == 0); // if that fails, smth must have been allocated completely wrong
        assert(handle.index < _capacity);
        const u32 index = (u32)(handle.cpu.ptr - _cpu_start.ptr) / _descriptor_size;
        assert(handle.index == index);

        // handles can be used by multiple frames, we have to deffer this operation until these resources are no longer needed by GPU or used by 
        // any shader
        const u32 frame_index = core::get_current_frame_index();
        _deferred_free_indices[frame_index].push_back(index); // remember which descriptor handles should be removed later.
        core::set_deferred_releases_flag();
        handle = {};
    }
}