#pragma once

#include "CommonHeaders.h"
#include "Window.h"

namespace mage::platform {
    struct window_init_info;

    // even if pointer is a nullptr, we will still be able to open a new window
    
    /// <summary>
    /// Adds window_id to our internal windows array/list if system call to CreateWindow succeeded.
    /// </summary>
    /// <param name="init_info"></param>
    /// <returns>window instance which under the hood manages a system's window.</returns>
    window create_window(const window_init_info* const init_info = nullptr);
    
    
    /// <summary>
    /// Destroys a window and frees the index by adding it to internal available_slots array.
    /// </summary>
    /// <param name="id">window_id pointing at the window we want to destroy.</param>
    void remove_window(window_id id);
}