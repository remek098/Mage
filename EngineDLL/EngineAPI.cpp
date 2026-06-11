#include "Common.h"
#include "CommonHeaders.h"
#include "..\Engine\Components\Script.h"
#include "..\Graphics\Renderer.h"
#include "..\Platform\PlatformTypes.h"
#include "..\Platform\Platform.h"

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

    utl::vector<gfx::render_surface> render_surfaces;
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

MAGE_ED_INTERFACE u32 CreateRenderSurface(HWND host, i32 width, i32 height) {
    assert(host);
    // no callback, no caption for a window inside editor; top_left = (0,0)
    platform::window_init_info info{ nullptr, host, nullptr, 0, 0, width, height };
    // NOTE: no gfx::surface until we have an implementation for graphics renderer
    gfx::render_surface surface{ platform::create_window(&info), {} };
    assert(surface.window.is_valid());

    render_surfaces.emplace_back(surface);
    return (u32)render_surfaces.size() - 1;
}

MAGE_ED_INTERFACE void RemoveRenderSurface(u32 id) {
    assert(id < render_surfaces.size());
    platform::remove_window(render_surfaces[id].window.get_id());
    // NOTE: not removing items from the array of render_surfaces; will do so after having a free-list cointainer
}

MAGE_ED_INTERFACE HWND GetWindowHandle(u32 id) {
    assert(id < render_surfaces.size());
    return (HWND)render_surfaces[id].window.handle();
}

MAGE_ED_INTERFACE void ResizeRenderSurface(u32 id) {
    assert(id < render_surfaces.size());
    render_surfaces[id].window.resize(0, 0);
    // NOTE: not removing items from the array of render_surfaces; will do so after having a free-list cointainer
}