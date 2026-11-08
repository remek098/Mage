#include "Platform/PlatformTypes.h"
#include "Platform/Platform.h"
#include "Graphics/Renderer.h"
#include "Graphics/Direct3D12/D3D12Core.h"
#include "Content/ContentToEngine.h"
#include "Components/Entity.h"
#include "Components/Transform.h"
#include "Components/Script.h"
#include "TestRenderer.h"
#include "ShaderCompilation.h"

#include <fstream>
#include <filesystem>

//#include <vector>
//#include <thread>
#include <Windows.h>
#include <timeapi.h>
#pragma comment(lib, "winmm.lib")

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
struct camera_surface {
    game_entity::entity entity{};
    gfx::camera camera{};
    gfx::render_surface surface{};
} camera;

//id::id_type item_id{ id::invalid_id };
//id::id_type model_id{ id::invalid_id };

camera_surface g_surfaces[4];
time_it timer{};


bool is_restarting = false;
bool resized = false;
// forward declerations
void destroy_camera_surface(camera_surface& surface);
bool test_initialize();
void test_shutdown();
void generate_lights();
void remove_lights();

void create_render_items();
void destroy_render_items();
void get_render_items(id::id_type* items, u32 count);

LRESULT win_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    bool toggle_fullscreen = false;

    switch (msg) {
        case WM_DESTROY:
        {
            bool all_closed = true;
            for (u32 i = 0; i < _countof(g_surfaces); ++i) {
                if (g_surfaces[i].surface.window.is_valid()) {
                    if (g_surfaces[i].surface.window.is_closed()) {
                        destroy_camera_surface(g_surfaces[i]);
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
            if (win.get_id() == g_surfaces[i].surface.window.get_id()) {
                if (toggle_fullscreen) {
                    win.set_fullscreen(!win.is_fullscreen());
                    // The default window procedure will play a system notification sound when pressing
                    // the Alt+Enter if WM_SYSCHAR is not handled.
                    // By returning 0 we tell the system that we handled this message.
                    return 0;
                }
                else {
                    g_surfaces[i].surface.surface.resize(win.width(), win.height());
                    g_surfaces[i].camera.aspect_ratio((f32)win.width() / win.height());
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

game_entity::entity
create_one_game_entity(math::vec3 position, math::vec3 rotation, const char* script_name) {
    using namespace DirectX;
    transform::init_info transform_info{};
    //math::vec3a rot{ 0.f, is_camera ? 3.14f : 0.f, 0 };
    // math::vec3a rot{ 0.f, 3.14159f, 0.f };
    XMVECTOR quat{ XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&rotation))};
    math::vec4a rot_quat;
    XMStoreFloat4A(&rot_quat, quat);
    memcpy(&transform_info.rotation[0], &rot_quat.x, sizeof(transform_info.rotation));
    memcpy(&transform_info.position[0], &position.x, sizeof(transform_info.position));


    script::init_info script_info{};
    if (script_name) {
        script_info.script_creator = script::detail::get_script_creator(script::detail::string_hash()(script_name));
        assert(script_info.script_creator);
    }

    // just so it is outside of geometry we try to render
    /*if (is_camera) {
        transform_info.position[1] = 1.f;
        transform_info.position[2] = 3.f;
    }*/

    game_entity::entity_info entity_info{};
    entity_info.tranform = &transform_info;
    entity_info.script = &script_info;
    game_entity::entity ntt{ game_entity::create(entity_info) };
    assert(ntt.is_valid());
    return ntt;
}

void
remove_game_entity(game_entity::entity_id id) {
    game_entity::remove(id);
}

void create_camera_surface(camera_surface& surface, platform::window_init_info info) {
    surface.surface.window = platform::create_window(&info);
    surface.surface.surface = gfx::create_surface(surface.surface.window);
    // surface.entity = create_one_game_entity({/*-2.4f, -2.0f, 0.5f*/ 5.02f, 1.8f, 3.43f }, {/*0.f, 3.14f, 0.f*/ 0.2f, -2.15f, 0.f}, nullptr);
    // surface.entity = create_one_game_entity({ 4.f, 1.8f, -10.59f }, { 0.2f, -0.1f, 0.f }, nullptr);
    surface.entity = create_one_game_entity(
        { 0.0f, 3.5f, 15.0f },    // Position: Centered horizontally, slightly elevated, out in front on positive Z
        { 0.15f, 3.14159f, 0.f }, // Rotation: Yawned ~180 degrees (PI radians) to face backward toward the interior
        "camera_script"
    );
    surface.camera = gfx::create_camera(gfx::perspective_camera_init_info{ surface.entity.get_id() });
    surface.camera.aspect_ratio((f32)surface.surface.window.width() / surface.surface.window.height());
}

void destroy_camera_surface(camera_surface& surface) {
    camera_surface temp = surface;
    surface = {};

    if(temp.surface.surface.is_valid()) gfx::remove_surface(temp.surface.surface.get_id());
    if(temp.surface.window.is_valid()) platform::remove_window(temp.surface.window.get_id());
    if (temp.camera.is_valid()) gfx::remove_camera(temp.camera.get_id());
    if (temp.entity.is_valid()) game_entity::remove(temp.entity.get_id());
}

bool test_initialize() {
    while (!compile_shaders()) {
        // pop up a message box allowing the user to retry compilation.
        if (MessageBox(nullptr, L"Failed to compile engine shaders.", L"Shader Compilation Error.", MB_RETRYCANCEL) != IDRETRY)
            return false;
    }

    if (!gfx::initialize(gfx::gfx_platform::d3d12)) {
        MessageBox(nullptr, L"Gfx Init Failed", L"Error", MB_OK);
        return false;
    }

    platform::window_init_info info[] = {
            {&win_proc, nullptr, L"Test renderer window 1", 0, 0, 400, 800},
            {&win_proc, nullptr, L"Test renderer window 2", 150, 150, 600, 400},
            {&win_proc, nullptr, L"Test renderer window 3", 200, 200, 400, 400},
            {&win_proc, nullptr, L"Test renderer window 4", 250, 250, 800, 600}
    };
    static_assert(_countof(info) == _countof(g_surfaces));

    for (u32 i = 0; i < _countof(g_surfaces); ++i)
        create_camera_surface(g_surfaces[i], info[i]);


    // load test model
    // std::unique_ptr<u8[]> model;
    // u64 size{ 0 };
    // if (!read_file("..\\..\\enginetest\\model.model", model, size)) return false;

    //model_id = content::create_resource(model.get(), content::asset_type::mesh);
    //if (!id::is_valid(model_id)) return false;

    init_test_workers(buffer_test_worker);

    // centered in the world and rotating around the Y axis
    create_render_items();

    generate_lights();

    is_restarting = false;
    return true;
}

void test_shutdown() {
    remove_lights();
    destroy_render_items();
    join_test_workers();

    // if (id::is_valid(model_id)) content::destroy_resource(model_id, content::asset_type::mesh);

    for (u32 i = 0; i < _countof(g_surfaces); ++i)
        destroy_camera_surface(g_surfaces[i]);

    gfx::shutdown();
}

bool EngineTest::initialize() {
    // because windows is funny and not doing it would make std::this_thread::sleep_for(std::chrono::milliseconds(10)); actually sleep for 15.6 ms
    // for some unknown reasons. I guess power optimizations or smth ...
    timeBeginPeriod(1);
    return test_initialize();
}

void EngineTest::run() {
    timer.begin();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    script::update(timer.dt_avg());
    for (u32 i = 0; i < _countof(g_surfaces); ++i) {
        if (g_surfaces[i].surface.surface.is_valid()) {
            f32 threshold[]{ 10, 10 };

            id::id_type render_items[2]{};
            get_render_items(&render_items[0], 2);

            gfx::frame_info info{};
            info.render_item_ids = &render_items[0];
            info.render_item_count = 2;
            info.thresholds = threshold;
            info.light_set_key = 0;
            info.avarage_frame_time = timer.dt_avg();
            info.camera_id = g_surfaces[i].camera.get_id();

            // swapchain presentations get blocked causing e.g. 4 rendered surfaces to lock application to 34 fps,
            // but when closing 2 of the windows, we are back to 66-67 fps sync.
            // Might want to delegate surface rendering to seperate threads.
            g_surfaces[i].surface.surface.render(info);
        }
    }


    /*
    NOTE:
    in order to make setup below work and actually run seperate threads to render surfaces,
    we would need to change rendering pipeline (D3D12Core.cpp) from global d3d12_command gfx_command instance to
    Independent Command Contexts per Thread 
    */
    
    /*
    f32 threshold[]{ 10, 10 };
    id::id_type render_items[2]{};
    get_render_items(&render_items[0], 2);

    std::vector<std::jthread> render_threads;
    render_threads.reserve(_countof(g_surfaces));

    for (u32 i = 0; i < _countof(g_surfaces); ++i) {
        if (g_surfaces[i].surface.surface.is_valid()) {
            // Emplace a thread that handles this specific surface index
            render_threads.emplace_back([this, i, thresholds = threshold, &render_items](std::stop_token st) {
                gfx::frame_info info{};
                info.render_item_ids = &render_items[0];
                info.render_item_count = 2;
                info.thresholds = thresholds;
                info.light_set_key = 0;
                info.avarage_frame_time = timer.dt_avg();
                info.camera_id = g_surfaces[i].camera.get_id();

                g_surfaces[i].surface.surface.render(info);
            });
        }
    }
    render_threads.clear();
    */

    timer.end();
}

void EngineTest::shutdown() {
    test_shutdown();
    timeEndPeriod(1);
}

#endif // TEST_RENDERER