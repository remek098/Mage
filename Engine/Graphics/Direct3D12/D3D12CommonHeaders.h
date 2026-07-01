#pragma once

#include "CommonHeaders.h"
#include "Graphics\Renderer.h"

#include <dxgi1_6.h>
#include <d3d12.h>
#include <wrl.h>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")

namespace mage::gfx::d3d12 {
    constexpr u32 frame_buffer_count = 3;
}

// assert that COM call to D3D API was successful
#ifdef _DEBUG
#ifndef DXCALL
#define DXCALL(x)                               \
if(FAILED(x)) {                                 \
    char line_number[32];                       \
    sprintf_s(line_number, "%u", __LINE__);     \
    OutputDebugStringA("Error in: ");           \
    OutputDebugStringA(__FILE__);               \
    OutputDebugStringA("\nLine:" );             \
    OutputDebugStringA(line_number);            \
    OutputDebugStringA("\n");                   \
    OutputDebugStringA(#x);                     \
    OutputDebugStringA("\n" );                  \
    __debugbreak();                             \
}                                               
#endif // !DXCALL
#else
#define DXCALL(x) x
#endif // _DEBUG

#ifdef _DEBUG
#define NAME_D3D12_OBJECT(obj, name) obj->SetName(name); OutputDebugString(L"::D3D12 Object Created: "); OutputDebugString(name); OutputDebugString(L"\n");
// indexed variant will include index in the name of the object
#define NAME_D3D12_OBJECT_INDEXED(obj, index, name)                 \
{                                                                   \
wchar_t full_name[128];                                             \
if(swprintf_s(full_name, L"%s[%u]", name, index) > 0) {             \
    obj->SetName(full_name);                                        \
    OutputDebugString(L"::D3D12 Object Created: ");                 \
    OutputDebugString(full_name);                                   \
    OutputDebugString(L"\n");                                       \
}}
#else
#define NAME_D3D12_OBJECT(obj, name) 
#define NAME_D3D12_OBJECT_INDEXED(obj, index, name)
#endif // _DEBUG

