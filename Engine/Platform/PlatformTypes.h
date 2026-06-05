#pragma once

#include "CommonHeaders.h"

#ifdef _WIN64

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

namespace mage::platform {
    // for WndProc https://learn.microsoft.com/en-us/windows/win32/api/winuser/nc-winuser-wndproc
    using window_proc = LRESULT(*)(HWND, UINT, WPARAM, LPARAM);
    using window_handle = HWND;

    struct window_init_info {
        window_proc         callback{ nullptr };
        window_handle       parent{ nullptr };
        const wchar_t*      caption{ nullptr };
        i32                 left{ 0 };
        i32                 top{ 0 };
        i32                 width{ 1920 };
        i32                 height{ 1080 };
    };
}

#endif