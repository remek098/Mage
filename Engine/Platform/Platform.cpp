#include "Platform.h"
#include "PlatformTypes.h"

namespace mage::platform {
#ifdef _WIN64

    namespace {
        // LPCWSTR main_class_name = L"MageWindow";
        struct window_info {
            HWND        hwnd{ nullptr };
            // stores info about RECT when not being in full screen mode
            RECT        client_area{ 0, 0, 1920, 1080 };
            RECT        fullscreen_area{};
            POINT       top_left{ 0,0 };
            DWORD       style{ WS_VISIBLE };
            bool        is_fullscreen{ false };
            bool        is_closed{ false };
        };

        utl::vector<window_info> windows;
        // -------------------------------------------------------------------------------------
        // TODO: this part should be handled by a free-list container
        
        // keeps slots available after removing window from windows array (or more so indexes that are free to take)
        utl::vector<u32> available_slots;


        u32 add_to_windows(window_info info) {
            u32 id = u32_invalid_id;
            if ( available_slots.empty() ) {
                id = (u32)windows.size();
                windows.emplace_back(info); // add new elements if there's none yet
            }
            else {
                // if there're free slots, just re-use them
                id = available_slots.back();
                available_slots.pop_back();
                assert(id != u32_invalid_id);
                windows[id] = info;
            }

            return id;
        }


        void remove_from_windows(u32 id) {
            assert(id < windows.size());
            available_slots.emplace_back(id);
        }
        // -------------------------------------------------------------------------------------

        window_info& get_window_from_id(window_id id) {
            assert(id < windows.size());
            assert(windows[id].hwnd);
            return windows[id];
        }

        window_info& get_window_from_handle(window_handle handle) {
            const window_id id{ (id::id_type)GetWindowLongPtr(handle, GWLP_USERDATA) };
            return get_window_from_id(id);
        }

        LRESULT CALLBACK internal_window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
            window_info* info{ nullptr };
            switch ( msg ) {
            case WM_DESTROY:
                get_window_from_handle(hwnd).is_closed = true;
                break;

            case WM_EXITSIZEMOVE:
                info = &get_window_from_handle(hwnd);
                break;

            // handled mainly because Maximizing a window doesn't proc WM_EXITSIZEMOVE message
            case WM_SIZE:
                if ( wparam == SIZE_MAXIMIZED ) {
                    info = &get_window_from_handle(hwnd);
                }
                break;
            
            // handled because going back to normal state of a window doesn't proc other messages above
            case WM_SYSCOMMAND:
                if ( wparam == SC_RESTORE ) { // when restoring a window
                    info = &get_window_from_handle(hwnd);
                }
                break;

            default:
                break;
            }

            // if info is not null, we might check for updates.
            if ( info ) {
                assert(info->hwnd);
                // if something happened to size, we update appropriate area/rect for us to read about the changes
                GetClientRect(info->hwnd, info->is_fullscreen ? &info->fullscreen_area : &info->client_area);

            }

            LONG_PTR long_ptr = GetWindowLongPtr(hwnd, 0);
            return long_ptr 
                ? ((window_proc)long_ptr)(hwnd, msg, wparam, lparam) 
                : DefWindowProc(hwnd, msg, wparam, lparam);
        }


        // --------------------   utility for window abstraction:
        void resize_window(const window_info& info, const RECT& rect) {
            // adjust the window size for correct device size
            RECT window_rect{ rect };
            AdjustWindowRect(&window_rect, info.style, FALSE);

            const i32 width = window_rect.right - window_rect.left;
            const i32 height = window_rect.bottom - window_rect.top;

            // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-movewindow
            MoveWindow(info.hwnd, info.top_left.x, info.top_left.y, width, height, true);
        }

        void resize_window(window_id id, u32 width, u32 height) {
            window_info& info = get_window_from_id(id);

            // checking just in case someone e.g. changes full screen resolution, e.g. throughout game settings
            RECT& area{ info.is_fullscreen ? info.fullscreen_area : info.client_area };
            area.bottom = area.top + height;
            area.right = area.left + width;

            resize_window(info, area);
        }

        void set_window_fullscreen(window_id id, bool is_fullscreen) {
            window_info& info = get_window_from_id(id);
            if ( info.is_fullscreen != is_fullscreen ) {
                info.is_fullscreen = is_fullscreen;
                
                if ( is_fullscreen ) {
                    // keep the data about window dimensions/size so they can get restored
                    // when switching out from fullscreen state.
                    GetClientRect(info.hwnd, &info.client_area);
                    // to get position of window before entering full screen mode
                    RECT rect;
                    GetWindowRect(info.hwnd, &rect);
                    info.top_left.x = rect.left;
                    info.top_left.y = rect.top;
                    info.style = 0; // no borders, no title bar when in full screen mode
                    SetWindowLongPtr(info.hwnd, GWL_STYLE, info.style);
                    ShowWindow(info.hwnd, SW_MAXIMIZE); // fills entire screen with no borders and title bar
                }
                else {
                    info.style = WS_VISIBLE | WS_OVERLAPPEDWINDOW; // setting it to "default" style as it was during window creation
                    SetWindowLongPtr(info.hwnd, GWL_STYLE, info.style);
                    resize_window(info, info.client_area);
                    ShowWindow(info.hwnd, SW_SHOWNORMAL);
                }
            }
        }

        bool is_window_fullscreen(window_id id) {
            return get_window_from_id(id).is_fullscreen;
        }


        // in _WIN64 window_handle is HWND
        window_handle get_window_handle(window_id id) {
            return get_window_from_id(id).hwnd;
        }

        void set_window_caption(window_id id, const wchar_t* caption) {
            window_info& info = get_window_from_id(id);
            SetWindowText(info.hwnd, caption);
        }

        math::u32vec4 get_window_size(window_id id) {
            window_info& info = get_window_from_id(id);
            RECT area{ info.is_fullscreen ? info.fullscreen_area : info.client_area };
            return { (u32)area.left, (u32)area.top, (u32)area.right, (u32)area.bottom };
        }

        bool is_window_closed(window_id id) {
            return get_window_from_id(id).is_closed;
        }

    } // anonymous namespace

    window create_window(const window_init_info* const init_info /* = nullptr */) {
        window_proc callback = init_info ? init_info->callback : nullptr;
        window_handle parent = init_info ? init_info->parent : nullptr;

        // https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-wndclassexa
        WNDCLASSEX wc;
        ZeroMemory(&wc, sizeof(wc));
        wc.cbSize = sizeof(WNDCLASSEX);
        // https://learn.microsoft.com/en-us/windows/win32/winmsg/window-class-styles
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = internal_window_proc;
        wc.cbClsExtra = 0; // no need for any extra bytes allocated.
        wc.cbWndExtra = callback ? sizeof(callback) : 0;
        wc.hInstance = 0;
        // default mouse cursor and application icon
        wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = CreateSolidBrush(RGB(26, 48, 76));
        wc.lpszMenuName = NULL;
        wc.lpszClassName = L"MageWindow";
        wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION); // small icon

        // technically should check if it doesn't fail, but it's ok
        RegisterClassEx(&wc);

        // create an instance of window class

        window_info info{};
        RECT rc{ info.client_area };

        // adjust the window size for the correct device size -> https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-adjustwindowrect
        AdjustWindowRect(&rc, info.style, FALSE);

        // unicode characters
        const wchar_t* caption = (init_info && init_info->caption) ? init_info->caption : L"Mage Game";
        const i32 left = (init_info && init_info->left) ? init_info->left : info.client_area.left;
        const i32 top = (init_info && init_info->top) ? init_info->top : info.client_area.top;
        const i32 width = (init_info && init_info->width) ? init_info->width : rc.right - rc.left;
        const i32 height = (init_info && init_info->height) ? init_info->height : rc.bottom - rc.top;

        // if we have parent, we're using it with a level editor, therefore we have to set it as a child window
        // otherwise, it's not a child window
        info.style |= parent ? WS_CHILD : WS_OVERLAPPEDWINDOW;

        info.hwnd = CreateWindowEx(
            0,                       // extended style
            wc.lpszClassName,        // window class name
            caption,                 // instanced window title
            info.style,              // window style
            left, top,               // initial window position X and Y
            width, height,           // initial window width and height
            parent,                  // handle to parent window
            NULL,                    // handle to menu
            NULL,                    // instance of this application
            NULL                     // extra creation params
        );

        if ( info.hwnd ) {
            SetLastError(0); // clearing it in case we try to register same window class again -> WeWindows stuff KappaChungusDeluxe

            // set long_ptr so that we can access window_id from internal_window_proc()
            const window_id id{ add_to_windows(info) };
            SetWindowLongPtr(info.hwnd, GWLP_USERDATA, (LONG_PTR)id);


            // set in the "extra" bytes the pointer to the window callback function -> will handle messages for window
            // NOTE: matches the way we get a long_ptr inside internal_window_proc()
            if(callback) SetWindowLongPtr(info.hwnd, 0, (LONG_PTR)callback);
            assert(GetLastError() == 0); // we got no error (S_OK)

            ShowWindow(info.hwnd, SW_SHOWNORMAL);
            UpdateWindow(info.hwnd);
            return window{ id };
        }

        return {};
    }

    void remove_window(window_id id) {
        window_info& info = get_window_from_id(id);
        DestroyWindow(info.hwnd);
        remove_from_windows(id);
    }

#elif
#error "You gotta implement at least one platform bruh"
#endif // _WIN64



    void window::set_fullscreen(bool is_fullscreen) const {
        assert(is_valid());
        set_window_fullscreen(_id, is_fullscreen);
    }

    bool window::is_fullscreen() const {
        assert(is_valid());
        return is_window_fullscreen(_id);
    }

    void* window::handle() const {
        assert(is_valid());
        return get_window_handle(_id);

    }

    void window::set_caption(const wchar_t* caption) const {
        assert(is_valid());
        set_window_caption(_id, caption);
    }

    const math::u32vec4 window::size() const {
        assert(is_valid());
        return get_window_size(_id);
    }

    void window::resize(u32 width, u32 height) const {
        assert(is_valid());
        resize_window(_id, width, height);
    }

    const u32 window::width() const {
        math::u32vec4 s = size();
        return s.z - s.x; // windows right - windows left
    }

    const u32 window::height() const {
        math::u32vec4 s = size();
        return s.w - s.y; // window bottom - window top
    }

    bool window::is_closed() const {
        assert(is_valid());
        return is_window_closed(_id);
    }

}