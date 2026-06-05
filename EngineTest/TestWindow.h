#pragma once
#include "Test.h"
#include "..\Platform\PlatformTypes.h"
#include "..\Platform\Platform.h"

using namespace mage;

platform::window g_windows[4];

LRESULT win_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch ( msg ) {
    case WM_DESTROY:
    {
        bool all_closed = true;
        for ( u32 i = 0; i < _countof(g_windows); ++i ) {
            if ( !g_windows[i].is_closed() ) {
                all_closed = false;
            }
        }
        if ( all_closed ) {
            PostQuitMessage(0);
            return 0;
        }

        break;
    }

    case WM_SYSCHAR:
    {
        // if alt + enter was pressed, we go to / exit full screen mode for a window
        if ( wparam == VK_RETURN && (HIWORD(lparam) & KF_ALTDOWN) ) {
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

class EngineTest : public Test {
public:
    bool initialize() override {
        platform::window_init_info info[] = {
            {&win_proc, nullptr, L"Test window 1", 100, 100, 400, 800},
            {&win_proc, nullptr, L"Test window 2", 150, 150, 600, 400},
            {&win_proc, nullptr, L"Test window 3", 200, 200, 400, 400},
            {&win_proc, nullptr, L"Test window 4", 250, 250, 800, 600}
        };
        static_assert(_countof(info) == _countof(g_windows));

        for ( u32 i = 0; i < _countof(g_windows); ++i )
            g_windows[i] = platform::create_window(&info[i]);

        return true;
    }

    void run() override {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    void shutdown() override {
        for ( u32 i = 0; i < _countof(g_windows); ++i )
            platform::remove_window(g_windows[i].get_id());
    }
};