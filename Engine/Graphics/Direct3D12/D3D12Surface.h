#pragma once
#include "D3D12CommonHeaders.h"
#include "D3D12Resources.h"

namespace mage::gfx::d3d12 {

    /// <summary>
    /// Our low-level representation of swapchain. Not to be confused with IDXGISwapchain
    /// </summary>
    class d3d12_surface {
    public:
        explicit d3d12_surface(platform::window window)
                    : _window(window)
        {
            assert(_window.handle());
        }

#if USE_STL_VECTOR
        DISABLE_COPY(d3d12_surface);
        constexpr d3d12_surface(d3d12_surface&& o)
            : _swapchain{ o._swapchain }, _window{ o._window }, _current_backbuffer_index{ o._current_backbuffer_index }
            , _viewport{ o._viewport }, _scissor_rect{ o._scissor_rect } 
        {
            for (u32 i = 0; i < frame_buffer_count; ++i) {
                _render_target_data[i].resource = o._render_target_data[i].resource;
                _render_target_data[i].rtv = o._render_target_data[i].rtv;
            }

            o.reset();
        }

        constexpr d3d12_surface& operator=(d3d12_surface&& o) {
            // NOTE: if you ever decided to for whatever reason overload & operator, use std::addressof() instead of &
            assert(this != &o);
            if (this != &o) {
                release();
                move(o);
            }

            return *this;
        }
#endif

        ~d3d12_surface() { release(); }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="factory"></param>
        /// <param name="cmd_queue"></param>
        /// <param name="format">Specify in what format will swapchain keep images. (i.e. render-target's format).</param>
        void create_swapchain(IDXGIFactory7* factory, ID3D12CommandQueue* cmd_queue, DXGI_FORMAT format);

        void present() const;
        void resize();

        constexpr u32 width() const { return (u32)_viewport.Width; }
        constexpr u32 height() const { return (u32)_viewport.Height; }
        constexpr ID3D12Resource* const get_backbuffer() { return _render_target_data[_current_backbuffer_index].resource; }
        constexpr D3D12_CPU_DESCRIPTOR_HANDLE rtv() const { return _render_target_data[_current_backbuffer_index].rtv.cpu; }
        constexpr const D3D12_VIEWPORT& viewport() const { return _viewport; }
        constexpr const D3D12_RECT& scissor_rect() const { return _scissor_rect; }

    private:
        void release();
        void finalize();

#if USE_STL_VECTOR
        constexpr void move(d3d12_surface& o) {
            _swapchain = o._swapchain;
            for (u32 i = 0; i < frame_buffer_count; ++i) {
                _render_target_data[i] = o._render_target_data[i];
            }
            _window = o._window;
            _current_backbuffer_index = o._current_backbuffer_index;
            _viewport = o._viewport;
            _scissor_rect = o._scissor_rect;

            o.reset();
        }
        /// <summary>
        /// overwrite every member with some default values without releasing anything.
        /// </summary>
        constexpr void reset() {
            _swapchain = nullptr;
            for (u32 i = 0; i < frame_buffer_count; ++i) {
                _render_target_data[i] = {};
            }
            _window = {};
            _current_backbuffer_index = 0;
            _viewport = {};
            _scissor_rect = {};
        }
#endif
    private:
        struct render_target_data {
            ID3D12Resource* resource = nullptr;
            descriptor_handle rtv{};
        };

        IDXGISwapChain4*            _swapchain = nullptr;
        render_target_data          _render_target_data[frame_buffer_count]{};
        platform::window            _window{};
        mutable u32                 _current_backbuffer_index = 0;
        D3D12_VIEWPORT              _viewport{};
        D3D12_RECT                  _scissor_rect{};
    };
}