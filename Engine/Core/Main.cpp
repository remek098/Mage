

#if _WIN64

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // !WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <crtdbg.h>

// only when not in editor mode
#ifndef USE_WITH_EDITOR

extern bool engine_initialize();
extern void engine_update();
extern void engine_shutdown();

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#if _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    if ( engine_initialize() ) {
        MSG msg;
        bool is_running = true;
        while ( is_running ) {
            // read and dispatch all WINDOWS messages until there're no messages left
            // to process (at least for the engine no more)
            while ( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) ) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);

                is_running &= (msg.message != WM_QUIT);
            }
            engine_update();
        }
    }

    engine_shutdown();
    return 0;
}

#endif // !USE_WITH_EDITOR


#endif