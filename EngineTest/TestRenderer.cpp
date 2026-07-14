#include "..\Platform\PlatformTypes.h"
#include "..\Platform\Platform.h"
#include "..\Graphics\Renderer.h"
#include "TestRenderer.h"
#include "ShaderCompilation.h"

#if TEST_RENDERER
using namespace mage;

gfx::render_surface g_surfaces[4];
time_it timer{};


bool is_restarting = false;
// forward declerations
void destroy_render_surface(gfx::render_surface& surface);
bool test_initialize();
void test_shutdown();

LRESULT win_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_DESTROY:
        {
            bool all_closed = true;
            for (u32 i = 0; i < _countof(g_surfaces); ++i) {
                if (g_surfaces[i].window.is_valid()) {
                    if (g_surfaces[i].window.is_closed()) {
                        destroy_render_surface(g_surfaces[i]);
                    }
                    else {
                        all_closed = false;
                    }

                }
            }
            if (all_closed && !is_restarting) {
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

        case WM_KEYDOWN:
        {
            if (wparam == VK_ESCAPE) {
                PostMessage(hwnd, WM_CLOSE, 0, 0);
                return 0;
            }
            else if (wparam == VK_F11) {
                is_restarting = true;
                test_shutdown();
                test_initialize();
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
    surface.surface = gfx::create_surface(surface.window);
}

void destroy_render_surface(gfx::render_surface& surface) {
    gfx::render_surface temp = surface;
    surface = {};

    if(temp.surface.is_valid()) gfx::remove_surface(temp.surface.get_id());
    if(temp.window.is_valid()) platform::remove_window(temp.window.get_id());
}

bool test_initialize() {
    while (!compile_shaders()) {
        // pop up a message box allowing the user to retry compilation.
        if (MessageBox(nullptr, L"Failed to compile engine shaders.", L"Shader Compilation Error.", MB_RETRYCANCEL) != IDRETRY)
            return false;
    }

    if (!gfx::initialize(gfx::gfx_platform::d3d12)) return false;

    platform::window_init_info info[] = {
            {&win_proc, nullptr, L"Test renderer window 1", 0, 0, 400, 800},
            {&win_proc, nullptr, L"Test renderer window 2", 150, 150, 600, 400},
            {&win_proc, nullptr, L"Test renderer window 3", 200, 200, 400, 400},
            {&win_proc, nullptr, L"Test renderer window 4", 250, 250, 800, 600}
    };
    static_assert(_countof(info) == _countof(g_surfaces));

    for (u32 i = 0; i < _countof(g_surfaces); ++i)
        create_render_surface(g_surfaces[i], info[i]);

    is_restarting = false;
    return true;
}

void test_shutdown() {
    for (u32 i = 0; i < _countof(g_surfaces); ++i)
        destroy_render_surface(g_surfaces[i]);

    gfx::shutdown();
}

bool EngineTest::initialize() {
    return test_initialize();
}

void EngineTest::run() {
    timer.begin();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    for (u32 i = 0; i < _countof(g_surfaces); ++i) {
        if (g_surfaces[i].surface.is_valid()) {
            g_surfaces[i].surface.render();
        }
    }
    timer.end();
}

void EngineTest::shutdown() {
    test_shutdown();
}

#endif // TEST_RENDERER