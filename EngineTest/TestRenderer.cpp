#include "..\Platform\PlatformTypes.h"
#include "..\Platform\Platform.h"
#include "..\Graphics\Renderer.h"
#include "..\Graphics\Direct3D12\D3D12Core.h"
#include "..\Content\ContentToEngine.h"
#include "TestRenderer.h"
#include "ShaderCompilation.h"

#include <fstream>
#include <filesystem>

#if TEST_RENDERER
using namespace mage;

//////////////////////////////////////////////////////////////////////////////////
#define ENABLE_TEST_WORKERS 0

constexpr u32 num_threads{ 8 };
bool          shutdown{ false };
std::thread   workers[num_threads];

utl::vector<u8> buffer(1024 * 1024, 0); // filled with zeroes, doesn't really matter since we're not using that data in any good way.
// test worker for upload context
void buffer_test_worker() {
    while (!shutdown) {
        auto* resource = gfx::d3d12::d3dx::create_buffer(buffer.data(), (u32)buffer.size());
        // NOTE: we can also use core::release(resource) since we're not using the buffer for rendering.
        //       However, this is a nice test for deferred_release functionality.
        gfx::d3d12::core::deferred_release(resource);
    }
}

template<class FnPtr, class... Args>
void init_test_workers(FnPtr&& fn_ptr, Args&&... args) {
#if ENABLE_TEST_WORKERS
    shutdown = false;
    for (auto& w : workers)
        w = std::thread(std::forward<FnPtr>(fn_ptr), std::forward<Args>(args)...);
#endif
}

void join_test_workers() {
#if ENABLE_TEST_WORKERS
    shutdown = true;
    for (auto& w : workers) w.join();
#endif
}
// Multithreading test worker spawn code /////////////////////////////////////////

id::id_type model_id{ id::invalid_id };

gfx::render_surface g_surfaces[4];
time_it timer{};


bool is_restarting = false;
bool resized = false;
// forward declerations
void destroy_render_surface(gfx::render_surface& surface);
bool test_initialize();
void test_shutdown();

LRESULT win_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    bool toggle_fullscreen = false;

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

        case WM_SIZE:
        {
            resized = (wparam != SIZE_MINIMIZED);
            break;
        }

        case WM_SYSCHAR:
        {
            // if alt + enter was pressed, we go to / exit full screen mode for a window
            toggle_fullscreen = (wparam == VK_RETURN && (HIWORD(lparam) & KF_ALTDOWN));
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

    // user is done and we can resize wwindow surface (at least with a mouse)
    if ((resized && GetAsyncKeyState(VK_LBUTTON) >= 0) || toggle_fullscreen) {
        platform::window win{ platform::window_id{(id::id_type)GetWindowLongPtr(hwnd, GWLP_USERDATA)} };
        for (u32 i = 0; i < _countof(g_surfaces); ++i) {
            if (win.get_id() == g_surfaces[i].window.get_id()) {
                if (toggle_fullscreen) {
                    win.set_fullscreen(!win.is_fullscreen());
                    // The default window procedure will play a system notification sound when pressing
                    // the Alt+Enter if WM_SYSCHAR is not handled.
                    // By returning 0 we tell the system that we handled this message.
                    return 0;
                }
                else {
                    g_surfaces[i].surface.resize(win.width(), win.height());
                    resized = false;
                }
                break;
            }
        }
    }

    return DefWindowProc(hwnd, msg, wparam, lparam);
}

/// <summary>
/// reads file from path, and sets data and size if binary file was opened.
/// </summary>
/// <param name="path"></param>
/// <param name="data"></param>
/// <param name="size"></param>
/// <returns></returns>
bool read_file(std::filesystem::path path, std::unique_ptr<u8[]>& data, u64& size) {
    if (!std::filesystem::exists(path)) return false;

    size = std::filesystem::file_size(path);
    assert(size);
    if (!size) return false;
    data = std::make_unique<u8[]>(size);
    std::ifstream file{ path, std::ios::in | std::ios::binary };
    if (!file || !file.read((char*)data.get(), size)) {
        file.close();
        return false;
    }

    file.close();
    return true;
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


    // load test model
    std::unique_ptr<u8[]> model;
    u64 size{ 0 };
    if (!read_file("..\\..\\enginetest\\model.model", model, size)) return false;

    model_id = content::create_resource(model.get(), content::asset_type::mesh);
    if (!id::is_valid(model_id)) return false;

    init_test_workers(buffer_test_worker);

    is_restarting = false;
    return true;
}

void test_shutdown() {
    join_test_workers();

    if (id::is_valid(model_id)) content::destroy_resource(model_id, content::asset_type::mesh);

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