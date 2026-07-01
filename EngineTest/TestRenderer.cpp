#include "..\Platform\PlatformTypes.h"
#include "..\Platform\Platform.h"
#include "..\Graphics\Renderer.h"
#include "TestRenderer.h"

#if TEST_RENDERER
using namespace mage;

gfx::render_surface g_surfaces[4];

LRESULT win_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_DESTROY:
        {
            bool all_closed = true;
            for (u32 i = 0; i < _countof(g_surfaces); ++i) {
                if (!g_surfaces[i].window.is_closed()) {
                    all_closed = false;
                }
            }
            if (all_closed) {
                PostQuitMessage(0);
                return 0;
            }

            break;
        }

        case WM_SYSCHAR:
        {
            // if alt + enter was pressed, we go to / exit full screen mode for a window
            if (wparam == VK_RETURN && (HIWORD(lparam) & KF_ALTDOWN)) {
                platform::window win{ platform::window_id{(id::id_type)GetWindowLongPtr(hwnd, GWLP_USERDATA)} };
                win.set_fullscreen(!win.is_fullscreen());
                return 0;
            }
            break;
        }
        default:
            break;
    }

    return DefWindowProc(hwnd, msg, wparam, lparam);
}

void create_render_surface(gfx::render_surface& surface, platform::window_init_info info) {
    surface.window = platform::create_window(&info);
}

void destroy_render_surface(gfx::render_surface& surface) {
    platform::remove_window(surface.window.get_id());

}

bool EngineTest::initialize() {
    bool result = gfx::initialize(gfx::gfx_platform::d3d12);
    if (!result) return result;

    platform::window_init_info info[] = {
            {&win_proc, nullptr, L"Test renderer window 1", 0, 0, 400, 800},
            {&win_proc, nullptr, L"Test renderer window 2", 150, 150, 600, 400},
            {&win_proc, nullptr, L"Test renderer window 3", 200, 200, 400, 400},
            {&win_proc, nullptr, L"Test renderer window 4", 250, 250, 800, 600}
    };
    static_assert(_countof(info) == _countof(g_surfaces));

    for (u32 i = 0; i < _countof(g_surfaces); ++i)
        create_render_surface(g_surfaces[i], info[i]);
    
    return result;
}

void EngineTest::run() {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    gfx::render();
}

void EngineTest::shutdown() {
    for (u32 i = 0; i < _countof(g_surfaces); ++i)
        destroy_render_surface(g_surfaces[i]);

    gfx::shutdown();
}

#endif // TEST_RENDERER