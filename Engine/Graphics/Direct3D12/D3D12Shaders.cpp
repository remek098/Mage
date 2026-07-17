#include "D3D12Shaders.h"
#include "Content\ContentLoader.h"

namespace mage::gfx::d3d12::shaders {
    namespace {
        // we want to represent our compiled shader as a memory chunk containing size and than actual byte_code that defines shader.
        typedef struct compiled_shader {
            u64         size;
            const u8*   byte_code;
        } const *compiled_shader_ptr; // typedeffing it, so we can use it to point to each shader.
                                      // const because we aren't allowed to write to this memory.

        /// each element in this array points to an offset within the shaders_blob.
        compiled_shader_ptr engine_shaders[engine_shader::count]{};


        // this is a chunk of memory that contains all compiled engine shaders.
        // The blob is an array of shader byte code consisting of a u64 size and an array of bytes.
        std::unique_ptr<u8[]> shaders_blob{};


        /// <summary>
        /// load all compiled shaders into one block of memory and then point to each shader using array of shaders.
        /// </summary>
        /// <returns></returns>
        bool load_engine_shaders() {
            assert(!shaders_blob);
            u64 size = 0;
            bool result = content::load_engine_shaders(shaders_blob, size);
            assert(shaders_blob && size);

            u64 offset = 0;
            u32 index = 0;
            while (offset < size && result) {
                assert(index < engine_shader::count);
                compiled_shader_ptr& shader{ engine_shaders[index] };
                assert(!shader); // well we're about to load shaders, shouldn't have any shaders loaded, obviously.
                result &= index < engine_shader::count && !shader;
                if (!result) break;

                shader = reinterpret_cast<const compiled_shader_ptr>(&shaders_blob[offset]);
                offset += sizeof(u64) + shader->size;
                ++index;
            }
            assert(offset == size && index == engine_shader::count);

            return result;
        } // bool load_engine_shaders()
    } // anonymous namespace


    bool initialize() {
        return load_engine_shaders();
    }

    void shutdown() {
        // clear pointers and release memory held by unique_ptr
        for (u32 i = 0; i < engine_shader::count; ++i) {
            engine_shaders[i] = {};
        }
        shaders_blob.reset();
    }

    D3D12_SHADER_BYTECODE get_engine_shader(engine_shader::id id) {

        assert(id < engine_shader::count);
        const compiled_shader_ptr shader{ engine_shaders[id] };
        assert(shader && shader->size);
        // return { &shader->byte_code, shader->size };
        return { &shader->byte_code, shader->size };
    }

} // namespace mage::gfx::d3d12::shaders