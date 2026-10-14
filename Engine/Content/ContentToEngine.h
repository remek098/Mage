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

    // get size of already existing buffer.
    constexpr u64 buffer_size() const { return sizeof(_byte_code_size) + hash_length + _byte_code_size; }

    // compute the required buffer size for a shader.
    constexpr static u64 buffer_size(u64 size) { return sizeof(_byte_code_size) + hash_length + size; }

private:
    u64       _byte_code_size;
    u8        _hash[hash_length];
    u8        _byte_code;
} const* compiled_shader_ptr; // typedeffing it, so we can use it to point to each shader.
// const because we aren't allowed to write to this memory.

struct lod_offset {
    u16 offset;
    u16 count;
};


id::id_type create_resource(const void* const data, asset_type::type type);
void destroy_resource(id::id_type id, asset_type::type type);

id::id_type add_shader_group(const u8* const* shaders, u32 num_shaders, const u32* const keys);
void remove_shader_group(id::id_type id);
compiled_shader_ptr get_shader(id::id_type id, u32 shader_key);

void get_submesh_gpu_ids(id::id_type geometry_content_id, u32 id_count, id::id_type* const gpu_ids);
void get_lod_offset(const id::id_type* const geometry_ids, const f32* const thresholds, u32 id_count,
                    utl::vector<lod_offset>& offsets);
}