#pragma once
#include "D3D12CommonHeaders.h"

namespace mage::gfx::d3d12 {
    // forward declaration
    class descriptor_heap;

    struct descriptor_handle {
        D3D12_CPU_DESCRIPTOR_HANDLE cpu{};
        D3D12_GPU_DESCRIPTOR_HANDLE gpu{};
        u32                         index = u32_invalid_id; // remember index of that slot in the heap, to check if cpu and gpu variables are still
                                                            // correct or not

        [[nodiscard]] constexpr bool is_valid() const { return cpu.ptr != 0; }
        [[nodiscard]] constexpr bool is_shader_visible() const { return gpu.ptr != 0; }


#ifdef _DEBUG
    private:
        friend class descriptor_heap;
        descriptor_heap*    container = nullptr; // to know on which descriptor heap out handle is being allocated
        //u32                 index = u32_invalid_id; // remember index of that slot in the heap, to check if cpu and gpu variables are still
        //                                            // correct or not
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

        [[nodiscard]] constexpr D3D12_DESCRIPTOR_HEAP_TYPE type() const { return _type; }
        [[nodiscard]] constexpr D3D12_CPU_DESCRIPTOR_HANDLE cpu_start() const { return _cpu_start; }
        [[nodiscard]] constexpr D3D12_GPU_DESCRIPTOR_HANDLE gpu_start() const { return _gpu_start; }
        [[nodiscard]] constexpr ID3D12DescriptorHeap* const heap() const { return _heap; }
        [[nodiscard]] const u32 capacity() { return _capacity; }
        [[nodiscard]] const u32 size() { return _size; }
        [[nodiscard]] const u32 descriptor_size() { return _descriptor_size; }
        
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
    }; // class descriptor_heap


    /// <summary>
    /// If you want to create committed resource (i.e. let d3d12 device create both resource and implicit heap that contains that resource)
    /// you should leave heap and resource pointers as nullptr.
    /// <para/>
    /// If you want to create placed resource (i.e. placed in specific already existing heap) you need to have heap pointer set.
    /// Also alloation_info should be set properly.
    /// resource pointer should be null, because we will have it set when creating texture.
    /// </summary>
    struct d3d12_texture_init_info {
        ID3D12Heap1*                        heap = nullptr;
        ID3D12Resource*                     resource = nullptr;
        D3D12_SHADER_RESOURCE_VIEW_DESC*    srv_desc = nullptr;
        D3D12_RESOURCE_DESC*                desc = nullptr;
        D3D12_RESOURCE_ALLOCATION_INFO1     allocation_info{};
        D3D12_RESOURCE_STATES               initial_state{};
        D3D12_CLEAR_VALUE                   clear_value;
    };

    class d3d12_texture {
    public:
        constexpr static u32 max_mips = 14; // support up to 8k resolution. (8192x8192)
        d3d12_texture() = default;
        explicit d3d12_texture(d3d12_texture_init_info info);
        // makes each instance of this class an unique instance, so there're no shared ownerships of a pointers contained
        // within this class.
        DISABLE_COPY(d3d12_texture);

        constexpr d3d12_texture(d3d12_texture&& o)
            : _resource(o._resource), _srv(o._srv)
        {
            // because we got simple data like pointers and integers in this class, we can get away with simply
            // copying values from other and reset the other. Normally you would std::move() data from other.
            o.reset();
        }

        constexpr d3d12_texture& operator=(d3d12_texture&& o) {
            assert(this != &o);
            if (this != &o) {
                release();
                move(o);
            }
            return *this;
        }

        ~d3d12_texture() { release(); }


        void release();
        [[nodiscard]] constexpr ID3D12Resource* const resource() const { return _resource; }
        [[nodiscard]] constexpr descriptor_handle srv() const { return _srv; }

    private:
        constexpr void move(d3d12_texture& o) {
            _resource = o._resource;
            _srv = o._srv;
            o.reset();
        }

        constexpr void reset() {
            _resource = nullptr;
            _srv = {};
        }

    private:
        ID3D12Resource*         _resource = nullptr;
        descriptor_handle       _srv; // because well, textures have to be accessible by shaders.
    };


    class d3d12_render_texture {
    public:
        d3d12_render_texture() = default;
        explicit d3d12_render_texture(d3d12_texture_init_info info);
        DISABLE_COPY(d3d12_render_texture);

        // move contstructor
        constexpr d3d12_render_texture(d3d12_render_texture&& o)
            : _texture{ std::move(o._texture) }, _mip_count{ o._mip_count }
        {
            for (u32 i = 0; i < _mip_count; ++i) _rtv[i] = o._rtv[i];
            o.reset();
        }

        constexpr d3d12_render_texture& operator=(d3d12_render_texture&& o) {
            assert(this != &o);
            if (this != &o) {
                release();
                move(o);
            }
            return *this;
        }

        ~d3d12_render_texture() { release(); }

        void release();


        [[nodiscard]] constexpr u32 mip_count() const { return _mip_count; }
        [[nodiscard]] constexpr D3D12_CPU_DESCRIPTOR_HANDLE rtv(u32 mip_index) const { assert(mip_index < _mip_count); return _rtv[mip_index].cpu; }
        [[nodiscard]] constexpr descriptor_handle srv() const { return _texture.srv(); }
        [[nodiscard]] constexpr ID3D12Resource* const resource() const { return _texture.resource(); }
    private:
        constexpr void move(d3d12_render_texture& o) {
            _texture = std::move(o._texture);
            _mip_count = o._mip_count;
            for (u32 i = 0; i < _mip_count; ++i) _rtv[i] = o._rtv[i];
            o.reset();
        }

        constexpr void reset() {
            for (u32 i = 0; i < _mip_count; ++i) _rtv[i] = {};
            _mip_count = 0;
            // we don't have to reset _texture, since it was already reset when moving from other instance.(check move constructor)
        }
    private:
        d3d12_texture           _texture{};
        descriptor_handle       _rtv[d3d12_texture::max_mips]{};
        u32                     _mip_count = 0; // depends on texture size, e.g. 32x32 texture can have up to 5 mip levels
    };


    class d3d12_depth_buffer {
    public:
        d3d12_depth_buffer() = default;

        /// <summary>
        /// info.srv_desc and info.resource have to be null for constructor to work properly and give us info straight from D3D12 API.
        /// <para/> Assumes that info.desc->Format is a format of depth buffer texture.
        /// </summary>
        /// <param name="info"></param>
        explicit d3d12_depth_buffer(d3d12_texture_init_info info);
        DISABLE_COPY(d3d12_depth_buffer);

        // move constructor
        constexpr d3d12_depth_buffer(d3d12_depth_buffer&& o) 
            : _texture{std::move(o._texture)}, _dsv{o._dsv}
        {
            o._dsv = {};
        }

        constexpr d3d12_depth_buffer& operator=(d3d12_depth_buffer&& o) {
            assert(this != &o);
            if (this != &o) {
                _texture = std::move(o._texture); // std::move already resets o._texture
                _dsv = o._dsv;
                o._dsv = {};
            }
            return *this;
        }

        ~d3d12_depth_buffer() { release(); }

        void release();


        [[nodiscard]] constexpr D3D12_CPU_DESCRIPTOR_HANDLE dsv() const { return _dsv.cpu; }
        [[nodiscard]] constexpr descriptor_handle srv() const { return _texture.srv(); }
        [[nodiscard]] constexpr ID3D12Resource* const resource() const { return _texture.resource(); }
    private:

    private:
        d3d12_texture           _texture{};
        descriptor_handle       _dsv{};
    };
}