#pragma once
#include "D3D12CommonHeaders.h"

namespace mage::gfx::d3d12 {
    // forward declaration
    class descriptor_heap;

    struct descriptor_handle {
        D3D12_CPU_DESCRIPTOR_HANDLE cpu{};
        D3D12_GPU_DESCRIPTOR_HANDLE gpu{};

        constexpr bool is_valid() const { return cpu.ptr != 0; }
        constexpr bool is_shader_visible() const { return gpu.ptr != 0; }

#ifdef _DEBUG
    private:
        friend class descriptor_heap;
        descriptor_heap*    container = nullptr; // to know on which descriptor heap out handle is being allocated
        u32                 index = u32_invalid_id; // remember index of that slot in the heap, to check if cpu and gpu variables are still
                                                    // correct or not
#endif
    };

    class descriptor_heap {
    public:
        explicit descriptor_heap(D3D12_DESCRIPTOR_HEAP_TYPE type) : _type(type) {}
        DISABLE_COPY_AND_MOVE(descriptor_heap);
        ~descriptor_heap() { assert(!_heap); }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="capacity">Has to be lower than D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2.</param>
        /// <param name="is_shader_visible"></param>
        /// <returns></returns>
        bool init(u32 capacity, bool is_shader_visible);
        void release();

        void process_deferred_free(u32 frame_index);

        [[nodiscard]] descriptor_handle allocate();
        void free(descriptor_handle& handle);

        constexpr D3D12_DESCRIPTOR_HEAP_TYPE type() const { return _type; }
        constexpr D3D12_CPU_DESCRIPTOR_HANDLE cpu_start() const { return _cpu_start; }
        constexpr D3D12_GPU_DESCRIPTOR_HANDLE gpu_start() const { return _gpu_start; }
        constexpr ID3D12DescriptorHeap* const heap() const { return _heap; }
        const u32 capacity() { return _capacity; }
        const u32 size() { return _size; }
        const u32 descriptor_size() { return _descriptor_size; }
        
        constexpr bool is_shader_visible() const { return _gpu_start.ptr != 0; }


    private:
        // chunk of memory; depending on whether it's shader visible or not, it is allocated in sys memory or GPU memory. That's why handles
        // are needed
        ID3D12DescriptorHeap*               _heap = nullptr;
        D3D12_CPU_DESCRIPTOR_HANDLE         _cpu_start{}; // basically unsigned 64-bit pointers
        D3D12_GPU_DESCRIPTOR_HANDLE         _gpu_start{};

        // pointer to the bufffer that has same length as capacity we're going to initializie with
        // will tell us indices to the heap to which we can allocate descriptors
        // using unique_ptr instead of vector becaause we need fixed number of elements.
        std::unique_ptr<u32[]>              _free_handles{};
        // for each frame when we try to free handles, we remember what these handles were, so we can properly deffer their freeing
        utl::vector<u32>                    _deferred_free_indices[frame_buffer_count]{};
        std::mutex                          _mutex{};

        u32                                 _capacity = 0; // how large heap is
        u32                                 _size = 0; // indicates for us how many descriptors have been created
        
        u32                                 _descriptor_size{};   // descriptor size for particular heap
        const D3D12_DESCRIPTOR_HEAP_TYPE    _type{};
    };
}