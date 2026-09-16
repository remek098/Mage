#pragma once
#include "CommonHeaders.h"

namespace mage::content {

struct asset_type {
    enum type : u32 {
        unknown             = 0,
        animation,
        audio,
        material,
        mesh,
        skeleton,
        texture,

        count
    };
};

// we want to represent our compiled shader as a memory chunk containing size and than actual byte_code that defines shader.
typedef struct compiled_shader {
    static constexpr u32 hash_length{ 16 };

    constexpr u64 byte_code_size() const { return _byte_code_size; }
    constexpr const u8* hash() const { return &_hash[0]; }
    constexpr const u8* byte_code() const { return &_byte_code; }

    constexpr u64 calc_size() const { return sizeof(u64) + hash_length + _byte_code_size; }

private:
    u64       _byte_code_size;
    u8        _hash[hash_length];
    u8        _byte_code;
} const* compiled_shader_ptr; // typedeffing it, so we can use it to point to each shader.
// const because we aren't allowed to write to this memory.




id::id_type create_resource(const void* const data, asset_type::type type);
void destroy_resource(id::id_type id, asset_type::type type);

id::id_type add_shader(const u8* data);
void remove_shader(id::id_type id);
compiled_shader_ptr get_shader(id::id_type id);
}