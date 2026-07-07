#include "D3D12Surface.h"
#include "D3D12Core.h"

namespace mage::gfx::d3d12 {
    namespace {
        /// <summary>
        /// In Direct3D 12, swapchain1 back buffer textures can't be directly created using an SRGB format.
        /// <para/> 
        /// If we need data to be SRGB, when we create views to back-buffers, 
        /// we can set descriptors in such a way that they will interpret UNORM as SRGB.
        /// </summary>
        /// <param name="format"></param>
        /// <returns></returns>
        constexpr DXGI_FORMAT to_non_srgb(DXGI_FORMAT format) {
            if (format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB) return DXGI_FORMAT_R8G8B8A8_UNORM;
            return format;
        }
    } // anonymous namespace


    void d3d12_surface::create_swapchain(IDXGIFactory7* factory, ID3D12CommandQueue* cmd_queue, DXGI_FORMAT format) {
        assert(factory && cmd_queue);
        release();

        if (SUCCEEDED(factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING,&_allow_tearing, sizeof(u32)))
                      && _allow_tearing) {
            _present_flags = DXGI_PRESENT_ALLOW_TEARING;
        }

        //_allow_tearing = _present_flags = 0;

        DXGI_SWAP_CHAIN_DESC1 desc{}; // using this to be able to set format as we please.
        desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        desc.BufferCount = buffer_count;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.Flags = _allow_tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
        desc.Format = to_non_srgb(format);
        desc.Width = _window.width();
        desc.Height = _window.height();
        // no MSAA -> basically we're copying finished image to render target.
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Scaling = DXGI_SCALING_STRETCH;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        desc.Stereo = false;

        IDXGISwapChain1* swapchain1;
        HWND hwnd = (HWND)_window.handle();
        DXCALL(factory->CreateSwapChainForHwnd(cmd_queue, hwnd, &desc, 
                                               nullptr, nullptr, &swapchain1));
        DXCALL(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER)); // make sure we can't do Alt+Enter to go to full screen
        DXCALL(swapchain1->QueryInterface(IID_PPV_ARGS(&_swapchain)));
        core::release(swapchain1);

        _current_backbuffer_index = _swapchain->GetCurrentBackBufferIndex();
        assert(core::rtv_heap().type() == D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        for (u32 i = 0; i < buffer_count; ++i) {
            _render_target_data[i].rtv = core::rtv_heap().allocate();
        }

        finalize();
    }

    void d3d12_surface::present() const {
        assert(_swapchain);
        DXCALL(_swapchain->Present(0, _present_flags));
        _current_backbuffer_index = _swapchain->GetCurrentBackBufferIndex();
    }

    void d3d12_surface::resize() {

    }


    void d3d12_surface::finalize() {
        // create RTVs for back-buffers
        for (u32 i = 0; i < buffer_count; ++i) {
            render_target_data& data = _render_target_data[i];
            assert(!data.resource); // we're creating swapchain, resource should be nullptr
            DXCALL(_swapchain->GetBuffer(i, IID_PPV_ARGS(&data.resource)));

            D3D12_RENDER_TARGET_VIEW_DESC desc{};
            desc.Format = core::default_render_target_format();
            desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            core::get_device()->CreateRenderTargetView(data.resource, &desc, data.rtv.cpu);
        }

        DXGI_SWAP_CHAIN_DESC desc{};
        DXCALL(_swapchain->GetDesc(&desc));
        const u32 width = desc.BufferDesc.Width;
        const u32 height = desc.BufferDesc.Height;
        assert(_window.width() == width && _window.height() == height);

        // set viewport and scissor rect
        _viewport.TopLeftX = 0.f;
        _viewport.TopLeftY = 0.f;
        _viewport.Width = (float)width;
        _viewport.Height = (float)height;
        _viewport.MinDepth = 0.f;
        _viewport.MaxDepth = 1.f;

        _scissor_rect = { 0, 0, (i32)width, (i32)height }; // {left, top, right, bottom}
    }
    

    void d3d12_surface::release() {
        for (u32 i = 0; i < buffer_count; ++i) {
            render_target_data& data = _render_target_data[i];
            core::release(data.resource);
            core::rtv_heap().free(data.rtv);
        }
        core::release(_swapchain);
    }
} // namespace mage::gfx::d3d12 