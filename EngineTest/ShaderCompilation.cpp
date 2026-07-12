#include "ShaderCompilation.h"

#include <d3d12shader.h>
#include <dxcapi.h>

#include "Graphics\Direct3D12\D3D12Core.h"
#include "Graphics\Direct3D12\D3D12Shaders.h"

#include <fstream>
#include <filesystem>

using namespace mage;
using namespace mage::gfx::d3d12::shaders;
using namespace Microsoft::WRL;

namespace {
    struct shader_file_info {
        const char*         file;
        const char*         function;
        engine_shader::id   id;
        shader_type::type   type;
    };

    constexpr shader_file_info shader_files[]{
        {"FullScreenTriangle.hlsl", "FullScreenTriangleVS", engine_shader::fullscreen_triangle_vs, shader_type::vertex},
        {"FillColor.hlsl", "FillColorPS", engine_shader::fill_color_ps, shader_type::pixel},
    };
    static_assert(_countof(shader_files) == engine_shader::count);

    // NOTE: hardcoded path to location of engine shaders.
    constexpr const char* shaders_source_path = "../../Engine/Graphics/Direct3D12/Shaders/";

    std::wstring to_wstring(const char* char_array) {
        std::string s{ char_array };
        return { s.begin(), s.end() };
    }

    class shader_compiler {
    public:
        shader_compiler() {
            HRESULT hr = S_OK;
            DXCALL(hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&_compiler)));
            if (FAILED(hr)) return;
            DXCALL(hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&_utils)));
            if (FAILED(hr)) return;
            DXCALL(hr = _utils->CreateDefaultIncludeHandler(&_include_handler));
            if (FAILED(hr)) return;
        }

        DISABLE_COPY_AND_MOVE(shader_compiler);

        IDxcBlob* compile(shader_file_info info, std::filesystem::path full_path) {
            assert(_compiler && _utils && _include_handler);
            HRESULT hr = S_OK;

            // load the source file using DxcUtils interface
            ComPtr<IDxcBlobEncoding> source_blob = nullptr;
            // for binary data, 2nd parameter in IDxcUtils::LoadFile() is NULL
            DXCALL(hr = _utils->LoadFile(full_path.c_str(), nullptr, &source_blob));
            if (FAILED(hr)) return nullptr;
            assert(source_blob && source_blob->GetBufferSize());

            std::wstring file = to_wstring(info.file);
            std::wstring func = to_wstring(info.function);
            std::wstring prof = to_wstring(_profile_strings[(u32)info.type]);

            // https://github.com/microsoft/DirectXShaderCompiler/wiki/Using-dxc.exe-and-dxcompiler.dll
            // we will use the example from above link as an inspiration :)
            LPCWSTR args[]{
                file.c_str(),                   // optional shader source file name for error reporting.
                L"-E", func.c_str(),            // entry point name
                L"-T", prof.c_str(),            // target profile -> to what type of shader we compile
                DXC_ARG_ALL_RESOURCES_BOUND,    // -all_resources_bound for dxc compiler, removes runtime safety checks 
#if _DEBUG
                DXC_ARG_DEBUG,
                DXC_ARG_SKIP_OPTIMIZATIONS,
#else
                DXC_ARG_OPTIMIZATION_LEVEL3,
#endif
                DXC_ARG_WARNINGS_ARE_ERRORS,
                L"-Qstrip_reflect",             // Strip reflection into a separate blob.
                L"-Qstrip_debug",               // Strip debug information into a separate blob.
            };
            OutputDebugStringA("Compiling ");
            OutputDebugStringA(info.file);

            return compile(source_blob.Get(), args, _countof(args));
        }

        IDxcBlob* compile(IDxcBlobEncoding* source_blob, LPCWSTR* args, u32 num_args) {
            DxcBuffer buffer{};
            buffer.Encoding = DXC_CP_ACP; // tell compiler to treat shader file data as raw binary or standard ANSI text.
            buffer.Ptr = source_blob->GetBufferPointer();
            buffer.Size = source_blob->GetBufferSize();

            HRESULT hr = S_OK;
            ComPtr<IDxcResult> results = nullptr;
            // https://learn.microsoft.com/en-us/windows/win32/api/dxcapi/nf-dxcapi-idxccompiler3-compile
            DXCALL(_compiler->Compile(&buffer, args, num_args, _include_handler.Get(), IID_PPV_ARGS(&results)));
            if (FAILED(hr)) return nullptr;

            ComPtr<IDxcBlobUtf8> errors = nullptr;
            hr = results->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);

            if (errors && errors->GetStringLength()) {
                OutputDebugStringA("\n Shader compilation error: \n");
                OutputDebugStringA(errors->GetStringPointer());
            }
            else {
                OutputDebugStringA(" [Succeeded] ");
            }
            OutputDebugStringA("\n");

            HRESULT status = S_OK;
            DXCALL(hr = results->GetStatus(&status)); // for reasons other than source code
            if (FAILED(hr) || FAILED(status)) return nullptr;

            ComPtr<IDxcBlob> shader = nullptr;
            DXCALL(hr = results->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shader), nullptr));
            if (FAILED(hr)) return nullptr;

            return shader.Detach(); // detaching pointer from ComPtr so that it's memory will not get Reset.
        }
    private:
        // NOTE: we have to use shader model 6.0 or later to be able to use DXC. AS and MS are supported only from SM6.5 onwards.
        constexpr static const char* _profile_strings[]{
            "vs_6_5", "hs_6_5", "ds_6_5", "gs_6_5", "ps_6_5", "cs_6_5", "as_6_5", "ms_6_5"
        };
        static_assert(_countof(_profile_strings) == shader_type::count);

        ComPtr<IDxcCompiler3>           _compiler = nullptr;
        ComPtr<IDxcUtils>               _utils = nullptr;
        ComPtr<IDxcIncludeHandler>      _include_handler = nullptr;
    };



    // Get the path to the compiled shaders binary file.
    decltype(auto) get_engine_shaders_path() {
        return std::filesystem::path{ gfx::get_engine_shaders_path(gfx::gfx_platform::d3d12) };
    }

    // If any file path of shader_files[] doesn't exist or last write time for shader's source file is > last compilation time, it returns false;
    // Otherwise true.
    bool compiled_shaders_up_to_date() {
        auto engine_shaders_path = get_engine_shaders_path();
        if (!std::filesystem::exists(engine_shaders_path)) return false;
        auto shaders_compilation_time = std::filesystem::last_write_time(engine_shaders_path);

        std::filesystem::path path{};
        std::filesystem::path full_path{};
        // check if either of engine shaders source files is newer than the compiled shader file.
        // In that case, we need to recompile.
        for (u32 i = 0; i < engine_shader::count; ++i) {
            auto& info = shader_files[i];

            path = shaders_source_path;
            path += info.file;
            full_path = path;
            if (!std::filesystem::exists(full_path)) return false;

            auto shader_file_time = std::filesystem::last_write_time(full_path);
            if (shader_file_time > shaders_compilation_time) {
                return false;
            }
        }

        return true;
    }

    bool save_compiled_shaders(utl::vector<ComPtr<IDxcBlob>>& shaders) {
        auto engine_shaders_path = get_engine_shaders_path();
        std::filesystem::create_directories(engine_shaders_path.parent_path());
        std::ofstream file(engine_shaders_path, std::ios::out | std::ios::binary);
        if (!file || !std::filesystem::exists(engine_shaders_path)) {
            file.close();
            return false;
        }

        // write size of shader first, then shader's Bytecode itself.
        for (auto& shader : shaders) {
            const D3D12_SHADER_BYTECODE byte_code{ shader->GetBufferPointer(), shader->GetBufferSize() };
            file.write((char*)&byte_code.BytecodeLength, sizeof(byte_code.BytecodeLength));
            file.write((char*)byte_code.pShaderBytecode, byte_code.BytecodeLength);
        }

        file.close();
        return true;
    }
} // anonymous namespace

bool compile_shaders() {
    if (compiled_shaders_up_to_date()) return true;
    utl::vector<ComPtr<IDxcBlob>> shaders;

    std::filesystem::path path{};
    std::filesystem::path full_path{};

    shader_compiler compiler{};

    OutputDebugStringA("\n \n \n");
    // compile shaders and then put all shaders together in a buffer in the same order as compilation went.
    for (u32 i = 0; i < engine_shader::count; ++i) {
        auto& info = shader_files[i];

        path = shaders_source_path;
        path += info.file;
        full_path = path;
        if (!std::filesystem::exists(full_path)) return false;

        ComPtr<IDxcBlob> compiled_shader{ compiler.compile(info, full_path) };
        if (compiled_shader && compiled_shader->GetBufferPointer() && compiled_shader->GetBufferSize()) {
            shaders.emplace_back(std::move(compiled_shader));
        }
        else {
            return false;
        }
    }
    OutputDebugStringA("\n \n \n");

    return save_compiled_shaders(shaders);
}