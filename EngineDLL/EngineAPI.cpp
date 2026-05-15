#include "Common.h"
#include "CommonHeaders.h"
#include "..\Engine\Components\Script.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // !WIN32_LEAN_AND_MEAN


#include <Windows.h>

using namespace mage;

namespace {
    HMODULE game_code_dll{ nullptr };

    // same signature as get_script_creator() in Engine
    using _get_script_creator = mage::script::detail::script_creator_fn_ptr(*)(size_t);
    _get_script_creator get_script_creator{ nullptr };

    // same signature as get_script_names() in Engine
    using _get_script_names = LPSAFEARRAY(*)(void);
    _get_script_names get_script_names{ nullptr };
}

MAGE_ED_INTERFACE u32 LoadGameCodeDll(const char* dll_path) {
    if ( game_code_dll ) return FALSE;
    game_code_dll = LoadLibraryA(dll_path);
    assert(game_code_dll);

    get_script_creator = (_get_script_creator)GetProcAddress(game_code_dll, "get_script_creator");
    get_script_names = (_get_script_names)GetProcAddress(game_code_dll, "get_script_names");

    return (game_code_dll && get_script_creator && get_script_names) ? TRUE : FALSE;
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


MAGE_ED_INTERFACE script::detail::script_creator_fn_ptr GetScriptCreator(const char* name) {
    // gives function pointer to a creation function for a script with a given name
    return (game_code_dll && get_script_creator) ? get_script_creator(script::detail::string_hash()(name)) : nullptr;
}

MAGE_ED_INTERFACE LPSAFEARRAY GetScriptNames() {
    return (game_code_dll && get_script_names) ? get_script_names() : nullptr;
}