#include "D3D12Core.h"
#include "D3D12Resources.h"
#include "D3D12Surface.h"
#include "D3D12Helpers.h"

using namespace Microsoft::WRL;

namespace mage::gfx::d3d12::core {
    namespace {
        /// <summary>
        /// instance of this class is allocated with placement new, beware of potential memory leaks if you don't destroy the default (in applications
        /// memory segment on app's initialization time) instance (especially if you include non-trivial data types that need to free memory on destroy)
        /// <para/>
        /// -> mage::gfx::d3d12::core::gfx_command variable in anonymous namespace
        /// </summary>
        class d3d12_command {
        public:
            d3d12_command() = default;
            DISABLE_COPY_AND_MOVE(d3d12_command);

            explicit d3d12_command(id3d12_device* const device, D3D12_COMMAND_LIST_TYPE type) {
                HRESULT hr = S_OK;
                
                D3D12_COMMAND_QUEUE_DESC desc{};
                desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
                desc.NodeMask = 0; // default GPU
                desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
                desc.Type = type;
                DXCALL(hr = device->CreateCommandQueue(&desc, IID_PPV_ARGS(&_cmd_queue)));
                if (FAILED(hr)) goto _error;
                NAME_D3D12_OBJECT(_cmd_queue,
                                  type == D3D12_COMMAND_LIST_TYPE_DIRECT ?
                                  L"Gfx Command Queue" :
                                  type == D3D12_COMMAND_LIST_TYPE_COMPUTE ?
                                  L"Compute Command Queue" : L"Command Queue");


                for (u32 i = 0; i < frame_buffer_count; ++i) {
                    command_frame& frame = _cmd_frames[i];
                    DXCALL(hr = device->CreateCommandAllocator(type, IID_PPV_ARGS(&frame.cmd_allocator)));
                    if (FAILED(hr)) goto _error;
                    NAME_D3D12_OBJECT_INDEXED(_cmd_frames[i].cmd_allocator, i,
                                      type == D3D12_COMMAND_LIST_TYPE_DIRECT ?
                                      L"Gfx Command Allocator" :
                                      type == D3D12_COMMAND_LIST_TYPE_COMPUTE ?
                                      L"Compute Command Allocator" : L"Command Allocator");
                }
                DXCALL(hr = device->CreateCommandList(0, type, _cmd_frames[0].cmd_allocator, nullptr, IID_PPV_ARGS(&_cmd_list)));
                if (FAILED(hr)) goto _error;
                DXCALL(_cmd_list->Close());
                NAME_D3D12_OBJECT(_cmd_list,
                                  type == D3D12_COMMAND_LIST_TYPE_DIRECT ?
                                  L"Gfx Command List" :
                                  type == D3D12_COMMAND_LIST_TYPE_COMPUTE ?
                                  L"Compute Command List" : L"Command List");

                DXCALL(hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence)));
                if (FAILED(hr)) goto _error;
                NAME_D3D12_OBJECT(_fence, L"D3D12 Fence");
                
                _fence_event = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
                assert(_fence_event);

                return;
            _error:
                release();
            }

            ~d3d12_command() {
                assert(!_cmd_queue && !_cmd_list && !_fence);
            }

            /// <summary>
            /// Wait for the current frame to be signaled and reset the command list/allocator.
            /// </summary>
            void begin_frame() {
                command_frame& frame = _cmd_frames[_frame_index];
                frame.wait(_fence_event, _fence);

                // NOTE: resetting command allocator will free the memory used by previously recorded commands.
                //       resetting command list will reopen it for recording new commands.
                DXCALL(frame.cmd_allocator->Reset());
                DXCALL(_cmd_list->Reset(frame.cmd_allocator, nullptr));
            }

            /// <summary>
            /// Signal the fence with the new fence value.
            /// </summary>
            void end_frame() {
                // subbmit command list(s) for execution
                DXCALL(_cmd_list->Close());
                ID3D12CommandList* const cmd_lists[] { _cmd_list };
                _cmd_queue->ExecuteCommandLists(_countof(cmd_lists), &cmd_lists[0]);

                u64& fence_value = _fence_value;
                ++fence_value;
                command_frame& frame = _cmd_frames[_frame_index];
                frame.fence_value = fence_value;
                _cmd_queue->Signal(_fence, fence_value);

                _frame_index = (_frame_index + 1) % frame_buffer_count;
            }

            void flush() {
                for (u32 i = 0; i < frame_buffer_count; ++i) {
                    _cmd_frames[i].wait(_fence_event, _fence);
                }
                _frame_index = 0;
            }

            void release() {
                flush();
                
                // clean everything related to fence
                core::release(_fence);
                _fence_value = 0;
                CloseHandle(_fence_event);
                _fence_event = nullptr;

                core::release(_cmd_queue);
                core::release(_cmd_list);
                
                // releases command allocators
                for (u32 i = 0; i < frame_buffer_count; ++i) {
                    _cmd_frames[i].release();
                }
            }

            constexpr ID3D12CommandQueue* const get_command_queue() const { return _cmd_queue; }
            constexpr id3d12_graphics_command_list* const command_list() const { return _cmd_list; }
            constexpr u32 frame_index() const { return _frame_index; }


        private:
            struct command_frame {
                ID3D12CommandAllocator* cmd_allocator   = nullptr;
                u64                     fence_value     = 0;

                void wait(HANDLE fence_event, ID3D12Fence1* fence) {
                    assert(fence && fence_event);

                    // if the current fence is still less than "fence value", then we know the GPU has not finished executing the command lists.
                    // since it has not reached the "_cmd_queue->Signal()" command
                    if (fence->GetCompletedValue() < fence_value) {
                        // in other words we're here if GPU hasn't caught up with our frame yet.
                        DXCALL(fence->SetEventOnCompletion(fence_value, fence_event));
                        WaitForSingleObject(fence_event, INFINITE);
                    }
                }

                void release() {
                    core::release(cmd_allocator);
                    fence_value = 0;
                }
            };

            ID3D12CommandQueue*             _cmd_queue          = nullptr;
            id3d12_graphics_command_list*   _cmd_list           = nullptr;
            ID3D12Fence1*                   _fence              = nullptr;
            u64                             _fence_value        = 0;
            HANDLE                          _fence_event        = nullptr;
            command_frame                   _cmd_frames[frame_buffer_count]{};
            u32                             _frame_index        = 0;
        }; // class d3d12_command

        using surface_collection = utl::free_list<d3d12_surface>;
// ---- list of variables in translation unit (in anonymous namespace)
        id3d12_device*                     d3d_main_device = nullptr;
        IDXGIFactory7*                      dxgi_factory = nullptr;
        d3d12_command                       gfx_command;
        surface_collection                  surfaces;

        // descriptor heaps
        descriptor_heap                     rtv_desc_heap{ D3D12_DESCRIPTOR_HEAP_TYPE_RTV };
        descriptor_heap                     dsv_desc_heap{ D3D12_DESCRIPTOR_HEAP_TYPE_DSV };
        descriptor_heap                     srv_desc_heap{ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV }; // that one is shader visible
        descriptor_heap                     uav_desc_heap{ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV }; // that will not be shader visible


        utl::vector<IUnknown*>              deferred_releases[frame_buffer_count]; // resources that have to be released for each frame buffer.
        u32                                 deferred_releases_flag[frame_buffer_count];
        std::mutex                          deferred_releases_mutex{};


        constexpr DXGI_FORMAT       render_target_format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        constexpr D3D_FEATURE_LEVEL minimum_feature_level{ D3D_FEATURE_LEVEL_11_0 };

        bool failed_init() {
            shutdown();
            return false;
        }


        // get the most performing adapter that supports our minimal feature level requirement.
        // NOTE: this function can be expanded in functionality with, e.g. checking if any output devices (i.e. screens) are attached, 
        //       enumerate the supported resolutions, provide means for the user to choose which adapter to use in a multi-adapter setting, etc.
        IDXGIAdapter4* determine_main_adapter() {
            IDXGIAdapter4* adapter = nullptr;
            // get adapters in descending order of performance
            for (u32 i = 0;
                 dxgi_factory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                                                          IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND;
                 ++i) {
                // pick the first adapter that supports the minimum feature level
                if (SUCCEEDED(D3D12CreateDevice(adapter, minimum_feature_level, __uuidof(ID3D12Device), nullptr))) {
                    return adapter;
                }
                release(adapter);
            }

            return nullptr;
        }

        D3D_FEATURE_LEVEL get_max_feature_level(IDXGIAdapter4* adapter) {
            constexpr D3D_FEATURE_LEVEL feature_levels[5]{
                D3D_FEATURE_LEVEL_11_0,
                D3D_FEATURE_LEVEL_11_1,
                D3D_FEATURE_LEVEL_12_0,
                D3D_FEATURE_LEVEL_12_1,
                D3D_FEATURE_LEVEL_12_2,
            };

            D3D12_FEATURE_DATA_FEATURE_LEVELS feature_level_info{};
            feature_level_info.NumFeatureLevels = _countof(feature_levels);
            feature_level_info.pFeatureLevelsRequested = feature_levels;

            ComPtr<ID3D12Device> device;
            DXCALL(D3D12CreateDevice(adapter, minimum_feature_level, IID_PPV_ARGS(&device)));
            DXCALL(device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &feature_level_info,
                                               sizeof(D3D12_FEATURE_DATA_FEATURE_LEVELS)));
            return feature_level_info.MaxSupportedFeatureLevel;
        }

        void __declspec(noinline) process_deferred_releases(u32 frame_index) {
            // noinline because we don't want bunch of instructions that are rarely executed in render() function 
            // so instead of jumping across bunch of instructions (that are loaded into cache), we just jump over a call instruction instead

            // make sure we're thread-safe -> object is unlocked when lock goes out of scope (so after function execution)
            std::lock_guard lock{ deferred_releases_mutex };

            // NOTE: we clear this flag at the beginning. If it'd be cleared at the end, then we might overwrite some other thread that was trying 
            //       to set it. It's fine if overwriting happens before processing items.
            deferred_releases_flag[frame_index] = 0; // clear flag for this frame

            rtv_desc_heap.process_deferred_free(frame_index);
            dsv_desc_heap.process_deferred_free(frame_index);
            srv_desc_heap.process_deferred_free(frame_index);
            uav_desc_heap.process_deferred_free(frame_index);
            
            // TODO: release pending resources -> for this frame
            utl::vector<IUnknown*>& resources{ deferred_releases[frame_index] };
            if (!resources.empty()) {
                for (auto& resource : resources) release(resource);
                resources.clear();
            }

        }
    } // anonymous namespace

    namespace detail {
        void deferred_release(IUnknown* resource) {
            const u32 frame_index = get_current_frame_index();
            // locking the function because we access deferred_releases, which will also be modified in process_deferred_releases
            std::lock_guard lock{ deferred_releases_mutex };
            deferred_releases[frame_index].push_back(resource);
            set_deferred_releases_flag(); // makes sure that for this frame thanks to process_deferred_releases() (called in render() each frame)
                                          // resources will be released at proper timing
        }
    } // namespace detail

    bool initialize() {
        // 2) determine what is the maximum feature level that is supported.
        // 3) Create ID3D12Device (virtual adapter)

        if (d3d_main_device) shutdown();

        u32 dxgi_factory_flags = 0;
#ifdef _DEBUG
        {
            ComPtr<ID3D12Debug6> debug_interface;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_interface)))) {
                debug_interface->EnableDebugLayer();
            }
            else {
                OutputDebugStringA("Warning: D3D12 Debug interface is not available. Verify that Graphics Tools optional feature is installed on this system.\n");
            }
            dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
        }
#endif // _DEBUG

        HRESULT hr = S_OK;
        DXCALL(hr = CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&dxgi_factory)));
        if (FAILED(hr)) return failed_init();

        // 1) determine which adapter (i.e. graphics hardware) to use
        ComPtr<IDXGIAdapter4> main_adapter;
        main_adapter.Attach(determine_main_adapter());
        if (!main_adapter) return failed_init();

        D3D_FEATURE_LEVEL max_feature_level{ get_max_feature_level(main_adapter.Get()) };
        assert(max_feature_level >= minimum_feature_level);
        if (max_feature_level < minimum_feature_level) return failed_init();

        DXCALL(hr = D3D12CreateDevice(main_adapter.Get(), max_feature_level, IID_PPV_ARGS(&d3d_main_device)));
        if (FAILED(hr)) return failed_init();

#ifdef _DEBUG
        {
            ComPtr<ID3D12InfoQueue1> info_queue;
            DXCALL(d3d_main_device->QueryInterface(IID_PPV_ARGS(&info_queue)));

            // if any of these occur, they will break the application
            info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
            info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
            info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
        }
#endif // _DEBUG

        // initialize descriptor heaps
        bool result = true;
        result &= rtv_desc_heap.init(512, false);
        result &= dsv_desc_heap.init(512, false);
        result &= srv_desc_heap.init(4096, true); // ofc in game number of textures and other things would be much bigger than this.
        result &= uav_desc_heap.init(512, false);
        if (!result) return failed_init();

        // NOTE: using placement new, because we construct in place and we don't want to ever be able to have it copied or moved
        // because our instances contain naked pointers, and we don't want that type of mess, oh no no....
        // basically placement new overrides everything that was already in place that has been reserved in data segment of the application.
        // but we got to make sure that we cannot have non-trivial types that allocate memory during initialization (e.g. utl::vector)
        // otherwise memory leaks inbound.
        new (&gfx_command) d3d12_command(d3d_main_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
        if (!gfx_command.get_command_queue()) return failed_init();

        NAME_D3D12_OBJECT(d3d_main_device, L"Main D3D12 Device");
        NAME_D3D12_OBJECT(rtv_desc_heap.heap(), L"RTV Descriptor Heap");
        NAME_D3D12_OBJECT(dsv_desc_heap.heap(), L"DSV Descriptor Heap");
        NAME_D3D12_OBJECT(srv_desc_heap.heap(), L"SRV Descriptor Heap");
        NAME_D3D12_OBJECT(uav_desc_heap.heap(), L"UAV Descriptor Heap");

        return true;
    }

    void shutdown() {
        gfx_command.release();

        // NOTE: we don't call process_deferred_releases at the end because some resources (such as swapchains)
        //       can't be released before their depending resources are released.
        for (u32 i = 0; i < frame_buffer_count; ++i) {
            process_deferred_releases(i);
        }

        release(dxgi_factory);

        rtv_desc_heap.release();
        dsv_desc_heap.release();
        srv_desc_heap.release();
        uav_desc_heap.release();

        // NOTE: some types only use deferred release for their resources during shutdown/reset/clear.
        //       To finally release these resources, we call process_deferred_releases once more.
        process_deferred_releases(0);

#ifdef _DEBUG
        {
            ComPtr<ID3D12InfoQueue1> info_queue;
            DXCALL(d3d_main_device->QueryInterface(IID_PPV_ARGS(&info_queue)));
            info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, false);
            info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);
            info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, false);
        }

        ComPtr<ID3D12DebugDevice2> debug_device;
        DXCALL(d3d_main_device->QueryInterface(IID_PPV_ARGS(&debug_device)));
        release(d3d_main_device);
        DXCALL(debug_device->ReportLiveDeviceObjects(
            D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL
        ));
#endif // _DEBUG

        release(d3d_main_device);
    }


    id3d12_device* const device() { return d3d_main_device; }

    descriptor_heap& rtv_heap() {return rtv_desc_heap;}
    descriptor_heap& dsv_heap() {return dsv_desc_heap;}
    descriptor_heap& srv_heap() {return srv_desc_heap;}
    descriptor_heap& uav_heap() { return uav_desc_heap; }


    u32 get_current_frame_index() { return gfx_command.frame_index(); }

    void set_deferred_releases_flag() { deferred_releases_flag[get_current_frame_index()] = 1; }



    surface create_surface(platform::window window) {
        // NOTE: not the best solution, will have to implement free-list and use it there.
        surface_id id{ surfaces.add(window) };
        surfaces[id].create_swapchain(dxgi_factory, gfx_command.get_command_queue(), render_target_format);
        
        return surface{ id };
    }
    void remove_surface(surface_id id) {
        gfx_command.flush();
        // TODO: will have to wait till we have a free-list container. (to do surfaces[id] = d3d12_surface{})
        //surfaces[id].release();
        surfaces.remove(id);
    }
    void resize_surface(surface_id id, u32 width, u32 height) {
        gfx_command.flush();
        surfaces[id].resize();

    }
    u32 surface_width(surface_id id) {
        return surfaces[id].width();
    }
    u32 surface_height(surface_id id) {
        return surfaces[id].height();
    }
    void render_surface(surface_id id) {
        // wait for the GPU to finish with the command allcator and reset the allocator once the GPU is done with it.
        // This frees the memory that was used to store commands.
        gfx_command.begin_frame();
        id3d12_graphics_command_list* cmd_list = gfx_command.command_list();

        const u32 frame_index = get_current_frame_index();
        if (deferred_releases_flag[frame_index]) {
            process_deferred_releases(frame_index);
        }

        const d3d12_surface& surface = surfaces[id];

        // presenting swapchain buffer happens in lockstep with frame buffers.
        surface.present();
        // record commands
        // ...

        // done recording commands. Now execute commands, signal and increment the fence value for next frame.
        gfx_command.end_frame();
    }

} // namespace mage::gfx::d3d12::core