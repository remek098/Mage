#include "Common.h"
#include "CommonHeaders.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // !WIN32_LEAN_AND_MEAN


#include <Windows.h>

using namespace mage;

namespace {
    HMODULE game_code_dll{ nullptr };
}

MAGE_ED_INTERFACE u32 LoadGameCodeDll(const char* dll_path) {
    if ( game_code_dll ) return FALSE;
    game_code_dll = LoadLibraryA(dll_path);
    assert(game_code_dll);

    return game_code_dll ?  TRUE : FALSE;
}

MAGE_ED_INTERFACE u32 UnloadGameCodeDll() {
    // of course return FALSE when there's no game_code_dll HMODULE present
    if ( !game_code_dll ) return FALSE;
    assert(game_code_dll);
    int result = FreeLibrary(game_code_dll);
    assert(result);
    game_code_dll = nullptr;
    return TRUE;
}