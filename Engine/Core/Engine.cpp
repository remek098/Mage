#if !defined(SHIPPING)

#include "..\Content\ContentLoader.h"
#include "..\Components\Script.h"
#include "..\Platform\PlatformTypes.h"
#include "..\Platform\Platform.h"
#include "..\Graphics\Renderer.h"

#include <thread>

using namespace mage;

namespace {
    gfx::render_surface game_window{};

    LRESULT game_win_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
        switch ( msg ) {
        case WM_DESTROY:
        {
            if ( game_window.window.is_closed() ) {
                PostQuitMessage(0);
                return 0;
            }

            break;
        }

        case WM_SYSCHAR:
        {
            // if alt + enter was pressed, we go to / exit full screen mode for a window
            if ( wparam == VK_RETURN && (HIWORD(lparam) & KF_ALTDOWN) ) {
                game_window.window.set_fullscreen(!game_window.window.is_fullscreen());
                return 0;
            }
            break;
        }
        default:
            break;
        }

        return DefWindowProc(hwnd, msg, wparam, lparam);
    }
}

bool engine_initialize() {
    if ( !mage::content::load_game() ) return false;

    // TODO: get the game name from the loaded game file
    platform::window_init_info info{
        &game_win_proc, nullptr, L"Mage engine game"
    };
    game_window.window = platform::create_window(&info);
    if ( !game_window.window.is_valid() ) return false;

    return true;
}

void engine_update() {
    mage::script::update(10.f);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void engine_shutdown() {
    platform::remove_window(game_window.window.get_id());
    mage::content::unload_game();
}

#endif // !defined(SHIPPING)