#include "ShaderCompilation.h"
#include <fstream>
#include <filesystem>

#include <d3d12shader.h>
#include <dxcapi.h>

#include "Graphics\Direct3D12\D3D12Core.h"
#include "Graphics\Direct3D12\D3D12Shaders.h"
#include "Content/ContentToEngine.h"
#include "Utilities/IOStreamUtils.h"

using namespace mage;
using namespace mage::gfx::d3d12::shaders;
using namespace Microsoft::WRL;

namespace {

// NOTE: hardcoded path to location of engine shaders.
constexpr const char* shaders_source_path = "../../Engine/Graphics/Direct3D12/Shaders/";

struct engine_shader_info {
    engine_shader::id   id;
    shader_file_info    info;
};


constexpr engine_shader_info engine_shader_files[]{
    { engine_shader::fullscreen_triangle_vs, {"FullScreenTriangle.hlsl", "FullScreenTriangleVS", shader_type::vertex} },
    { engine_shader::fill_color_ps, {"FillColor.hlsl", "FillColorPS", shader_type::pixel} },
    { engine_shader::post_process_ps, {"PostProcess.hlsl", "PostProcessPS", shader_type::pixel} },
};
static_assert(_countof(engine_shader_files) == engine_shader::count);


struct dxc_compiled_shader {
    ComPtr<IDxcBlob>        byte_code;
    ComPtr<IDxcBlobUtf8>    disassembly;
    DxcShaderHash           hash;
};

std::wstring 
to_wstring(const char* char_array) {
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

    dxc_compiled_shader compile(shader_file_info info, std::filesystem::path full_path) {
        assert(_compiler && _utils && _include_handler);
        HRESULT hr = S_OK;

        // load the source file using DxcUtils interface
        ComPtr<IDxcBlobEncoding> source_blob = nullptr;
        // for binary data, 2nd parameter in IDxcUtils::LoadFile() is NULL
        DXCALL(hr = _utils->LoadFile(full_path.c_str(), nullptr, &source_blob));
        if (FAILED(hr)) return {};
        assert(source_blob && source_blob->GetBufferSize());

        std::wstring file = to_wstring(info.file_name);
        std::wstring func = to_wstring(info.function);
        std::wstring prof = to_wstring(_profile_strings[(u32)info.type]);
        std::wstring inc = to_wstring(shaders_source_path);

        // https://github.com/microsoft/DirectXShaderCompiler/wiki/Using-dxc.exe-and-dxcompiler.dll
        // we will use the example from above link as an inspiration :)
        LPCWSTR args[]{
            file.c_str(),                   // optional shader source file name for error reporting.
            L"-E", func.c_str(),            // entry point name
            L"-T", prof.c_str(),            // target profile -> to what type of shader we compile
            L"-I", inc.c_str(),             // include path
            L"-enable-16bit-types",         // 16-bit int support in shaders.
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
        OutputDebugStringA(info.file_name);
        OutputDebugStringA(" : ");
        OutputDebugStringA(info.function);
        OutputDebugStringA("\n");

        return compile(source_blob.Get(), args, _countof(args));
    }

    dxc_compiled_shader compile(IDxcBlobEncoding* source_blob, LPCWSTR* args, u32 num_args) {
        DxcBuffer buffer{};
        buffer.Encoding = DXC_CP_ACP; // tell compiler to treat shader file data as raw binary or standard ANSI text.
        buffer.Ptr = source_blob->GetBufferPointer();
        buffer.Size = source_blob->GetBufferSize();

        HRESULT hr = S_OK;
        ComPtr<IDxcResult> results = nullptr;
        // https://learn.microsoft.com/en-us/windows/win32/api/dxcapi/nf-dxcapi-idxccompiler3-compile
        DXCALL(_compiler->Compile(&buffer, args, num_args, _include_handler.Get(), IID_PPV_ARGS(&results)));
        if (FAILED(hr)) return {};

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
        if (FAILED(hr) || FAILED(status)) return {};

        ComPtr<IDxcBlob> hash{ nullptr };
        DXCALL(hr = results->GetOutput(DXC_OUT_SHADER_HASH, IID_PPV_ARGS(&hash), nullptr));
        if (FAILED(hr)) return {};
        DxcShaderHash* const hash_buffer{ (DxcShaderHash* const)hash->GetBufferPointer() };
        // diffrent source code could result in the same byte code, so we only care about byte code hash.
        assert(!(hash_buffer->Flags & DXC_HASHFLAG_INCLUDES_SOURCE));
        OutputDebugStringA("Shader hash: ");
        for (u32 i{ 0 }; i < _countof(hash_buffer->HashDigest); ++i) {
            char hash_bytes[3]{}; // 2 chars for hex value + termination 0.
            sprintf_s(hash_bytes, "%02x", (u32)hash_buffer->HashDigest[i]);
            OutputDebugStringA(hash_bytes);
            OutputDebugStringA(" ");
        }
        OutputDebugStringA("\n");


        ComPtr<IDxcBlob> shader = nullptr;
        DXCALL(hr = results->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shader), nullptr));
        if (FAILED(hr)) return {};

        buffer.Ptr = shader->GetBufferPointer();
        buffer.Size = shader->GetBufferSize();

        ComPtr<IDxcResult>   disassemly_result{ nullptr };
        DXCALL(hr = _compiler->Disassemble(&buffer, IID_PPV_ARGS(&disassemly_result)));

        ComPtr<IDxcBlobUtf8> disassemly{ nullptr };
        DXCALL(hr = disassemly_result->GetOutput(DXC_OUT_DISASSEMBLY, IID_PPV_ARGS(&disassemly), nullptr));

        dxc_compiled_shader result{ shader.Detach(), disassemly.Detach() };
        memcpy(&result.hash.HashDigest[0], &hash_buffer->HashDigest[0], _countof(hash_buffer->HashDigest)); // BYTE array
        return result;
    }
private:
    // NOTE: we have to use shader model 6.0 or later to be able to use DXC. AS and MS are supported only from SM6.5 onwards.
    constexpr static const char* _profile_strings[]{
        "vs_6_6", "hs_6_6", "ds_6_6", "gs_6_6", "ps_6_6", "cs_6_6", "as_6_6", "ms_6_6"
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
bool 
compiled_shaders_up_to_date() {
    auto engine_shaders_path = get_engine_shaders_path();
    if (!std::filesystem::exists(engine_shaders_path)) return false;
    auto shaders_compilation_time = std::filesystem::last_write_time(engine_shaders_path);

    std::filesystem::path full_path{};
    // check if either of engine shaders source files is newer than the compiled shader file.
    // In that case, we need to recompile.
    for (u32 i = 0; i < engine_shader::count; ++i) {
        auto& file = engine_shader_files[i];

        full_path = shaders_source_path;
        full_path += file.info.file_name;
        if (!std::filesystem::exists(full_path)) return false;

        auto shader_file_time = std::filesystem::last_write_time(full_path);
        if (shader_file_time > shaders_compilation_time) {
            return false;
        }
    }

    return true;
}

bool 
save_compiled_shaders(utl::vector<dxc_compiled_shader>& shaders) {
    auto engine_shaders_path = get_engine_shaders_path();
    std::filesystem::create_directories(engine_shaders_path.parent_path());
    std::ofstream file(engine_shaders_path, std::ios::out | std::ios::binary);
    if (!file || !std::filesystem::exists(engine_shaders_path)) {
        file.close();
        return false;
    }

    // write size of shader first, then shader's Bytecode itself.
    for (auto& shader : shaders) {
        const D3D12_SHADER_BYTECODE byte_code{ shader.byte_code->GetBufferPointer(), shader.byte_code->GetBufferSize()};
        file.write((char*)&byte_code.BytecodeLength, sizeof(byte_code.BytecodeLength));
        file.write((char*)&shader.hash.HashDigest[0], _countof(shader.hash.HashDigest)); // BYTE array
        file.write((char*)byte_code.pShaderBytecode, byte_code.BytecodeLength);
    }

    file.close();
    return true;
}
} // anonymous namespace

std::unique_ptr<u8[]>
compile_shader(shader_file_info info, const char* file_path) {
    std::filesystem::path full_path{ file_path };
    full_path += info.file_name;
    if (!std::filesystem::exists(full_path)) return {};

    // https://github.com/Microsoft/DirectXShaderCompiler/issues/79
    // according to this issue thread, "creating compiler instances is pretty cheap, 
    // so it's probably not worth the hassle of caching / sharing them.
    shader_compiler compiler{};
    dxc_compiled_shader compiled_shader{ compiler.compile(info, full_path) };
    if (compiled_shader.byte_code && compiled_shader.byte_code->GetBufferPointer() && compiled_shader.byte_code->GetBufferSize()) {
        static_assert(content::compiled_shader::hash_length == _countof(DxcShaderHash::HashDigest));
        
        const u64 buffer_size{ sizeof(u64) + content::compiled_shader::hash_length + compiled_shader.byte_code->GetBufferSize() };
        std::unique_ptr<u8[]> buffer{ std::make_unique<u8[]>(buffer_size) };
        utl::blob_stream_writer blob{ buffer.get(), buffer_size };

        blob.write(compiled_shader.byte_code->GetBufferSize());
        blob.write(compiled_shader.hash.HashDigest, content::compiled_shader::hash_length);
        blob.write((u8*)compiled_shader.byte_code->GetBufferPointer(), compiled_shader.byte_code->GetBufferSize());

        assert(blob.offset() == buffer_size);
        return buffer;
    }
    return {};
}

bool 
compile_shaders() {
    if (compiled_shaders_up_to_date()) return true;

    shader_compiler compiler{};
    utl::vector<dxc_compiled_shader> shaders;
    std::filesystem::path full_path{};


    OutputDebugStringA("\n \n \n");
    // compile shaders and then put all shaders together in a buffer in the same order as compilation went.
    for (u32 i = 0; i < engine_shader::count; ++i) {
        auto& file = engine_shader_files[i];

        full_path = shaders_source_path;
        full_path += file.info.file_name;
        if (!std::filesystem::exists(full_path)) return false;

        dxc_compiled_shader compiled_shader{ compiler.compile(file.info, full_path) };
        if (compiled_shader.byte_code && compiled_shader.byte_code->GetBufferPointer() && compiled_shader.byte_code->GetBufferSize()) {
            shaders.emplace_back(std::move(compiled_shader));
        }
        else {
            return false;
        }
    }
    OutputDebugStringA("\n \n \n");

    return save_compiled_shaders(shaders);
}