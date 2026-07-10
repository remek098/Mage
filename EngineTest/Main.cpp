#pragma comment(lib, "Engine.lib")

#include "Test.h"

#if TEST_ENTITY_COMPONENTS
#include "TestEntityComponents.h"
#elif TEST_WINDOW
#include "TestWindow.h"
#elif TEST_RENDERER
#include "TestRenderer.h"
#else
#error One of the tests needs to be enabled
#endif


#ifdef _WIN64
#include <Windows.h>
#include <filesystem>

// TODO: duplicate
std::filesystem::path set_current_directory_to_exe_path() {
    // set the working directory to the .exe path
    wchar_t path[MAX_PATH];
    const uint32_t length = GetModuleFileName(0, &path[0], MAX_PATH);
    if (!length || GetLastError() == ERROR_INSUFFICIENT_BUFFER) return {};
    std::filesystem::path p{ path };
    std::filesystem::current_path(p.parent_path());
    return std::filesystem::current_path();
}


int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#if _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    
    set_current_directory_to_exe_path();

    EngineTest test{};
    if ( test.initialize() ) {
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
            test.run();
        }
    }

    test.shutdown();
    return 0;
}


#else
int main() {
#if _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    
    EngineTest test{};
    if ( test.initialize() ) {
        test.run();
    }
    test.shutdown();
}
#endif // _WIN64