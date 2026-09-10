#include "ContentToEngine.h"
#include "Graphics/Renderer.h"
#include "Utilities/IOStreamUtils.h"

namespace mage::content {

namespace {

// internal class for only reading our ugly ahh format that is liked by cpu cache
class geometry_hierarchy_stream {
public:
	struct lod_offset {
		u16 offset;
		u16 count;
	};
	DISABLE_COPY_AND_MOVE(geometry_hierarchy_stream);
	geometry_hierarchy_stream(u8* const buffer, u32 lods = u32_invalid_id) 
		: _buffer{buffer}
	{
		assert(buffer && lods);
		if (lods != u32_invalid_id) {
			// only needed if we're initializing buffer with hierarchy data
			*((u32*)buffer) = lods;
		}

		// these should match our output format stored in flat array, i.e.
		_lod_count = *((u32*)buffer);
		_thresholds = (f32*)(&buffer[sizeof(u32)]);
		_lod_offsets = (lod_offset*)(&_thresholds[_lod_count]);
		_gpu_ids = (id::id_type*)(&_lod_offsets[_lod_count]);
	}

	void gpu_ids(u32 lod, id::id_type*& ids, u32& id_count) {
		assert(lod < _lod_count);
		ids = &_gpu_ids[_lod_offsets[lod].offset];
		id_count = _lod_offsets[lod].count;
	}

	u32 lod_from_treshold(f32 treshold) {
		assert(treshold > 0);
		for (u32 i{ _lod_count - 1 }; i > 0; --i) {
			if (_thresholds[i] <= treshold) return i;
		}

		assert(false); // shouldn't ever get here.
		return 0;
	}

	[[nodiscard]] constexpr u32 lod_count() const { return _lod_count; }
	[[nodiscard]] constexpr f32* thresholds() const { return _thresholds; }
	[[nodiscard]] constexpr lod_offset* lod_offsets() const { return _lod_offsets; }
	[[nodiscard]] constexpr id::id_type* gpu_ids() const { return _gpu_ids; }

private:
	u8* const		_buffer;
	f32*			_thresholds; // pointer to array
	lod_offset*		_lod_offsets;
	id::id_type*	_gpu_ids;
	u32				_lod_count;
};

// this constant indicates an element in geometry_hierarchies is not a pointer, but a gpu_id
constexpr uintptr_t single_mesh_marker{ (uintptr_t)0x01 };
utl::free_list<u8*> geometry_hierarchies;
std::mutex			geometry_mutex;

// NOTE: expects the same data as create_geometry_resource()
u32 
get_geometry_hierarchy_buffer_size(const void* const data) {
	assert(data);
	utl::blob_stream_reader blob{ (const u8*)data };
	const u32 lod_count{ blob.read<u32>() };
	assert(lod_count);
	// add size of lod_count, tresholds and lod offsets to the size of hierarchy.
	u32 size{ sizeof(u32) + (sizeof(f32) + sizeof(geometry_hierarchy_stream::lod_offset)) * lod_count};

	for (u32 lod_idx{ 0 }; lod_idx < lod_count; ++lod_idx) {
		// skip treshold
		blob.skip(sizeof(f32));
		// add size of gpu_ids (sizeof(id::id_type) * submesh_count)
		size += sizeof(id::id_type) * blob.read<u32>();
		// skip submesh data and go to the next LOD
		blob.skip(blob.read<u32>());
	}
	return size;
}

// Creates a hierarchy stream for a geometry that has multiple LODs and/or multiple submeshes.
// NOTE: expects the same data as create_geometry_resource()
id::id_type 
create_mesh_hierarchy(const void* const data) {
	assert(data);
	const u32 size{ get_geometry_hierarchy_buffer_size(data) };
	u8* const hierarchy_buffer{ (u8* const)malloc(size) };

	utl::blob_stream_reader blob{ (const u8*)data };
	const u32 lod_count{ blob.read<u32>() };
	assert(lod_count);

	geometry_hierarchy_stream stream{ hierarchy_buffer, lod_count };
	u16 submesh_index{ 0 };
	id::id_type* const gpu_ids{ stream.gpu_ids() };

	for (u32 lod_idx{ 0 }; lod_idx < lod_count; ++lod_idx) {
		stream.thresholds()[lod_idx] = blob.read<f32>();
		const u32 id_count{ blob.read<u32>() };
		assert(id_count < (1 << 16));
		stream.lod_offsets()[lod_idx] = { submesh_index, (u16)id_count };
		blob.skip(sizeof(u32)); // skip over the size_of_submeshes
		for (u32 id_idx{ 0 }; id_idx < id_count; ++id_idx) {
			const u8* at{ blob.position() };
			gpu_ids[submesh_index++] = gfx::add_submesh(at);
			blob.skip((u32)(at - blob.position())); // skip the bytes that were read and written to gpu memory
			assert(submesh_index < (1 << 16));
		}
	}

	assert([&]() {
		f32 previous_threshold{ stream.thresholds()[0] };
		for (u32 i{ 1 }; i < lod_count; ++i) {
			if (stream.thresholds()[i] <= previous_threshold) return false;
			previous_threshold = stream.thresholds()[i];
		}
		return true;
		   }());

	static_assert(alignof(void*) > 2, "We need to least significant bit for a single mesh marker.");
	std::lock_guard lock{ geometry_mutex };
	return geometry_hierarchies.add(hierarchy_buffer);
}

// Creates a single submesh gpu_id
// NOTE: expects the same data as create_geometry_resource()
id::id_type
create_single_mesh(const void* const data) {
	assert(data);
	utl::blob_stream_reader blob{ (const u8*)data };
	// skip lod_count, lod_threshold, submesh_count and size_of_submeshes.
	blob.skip(sizeof(u32) + sizeof(f32) + sizeof(u32) + sizeof(u32));
	const u8* at{ blob.position() };
	const id::id_type gpu_id{gfx::add_submesh(at)};
	// abusing the fact from https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/malloc?view=msvc-170
	// The storage space pointed to by the return value is suitably aligned for storage of any type of object that has an alignment requirement 
	// less than or equal to that of the fundamental alignment. 
	// (In Visual C++, the fundamental alignment is the alignment that's required for a double, or 8 bytes.
	// In code that targets 64-bit platforms, it's 16 bytes.) -> and we target x64 platform only

	/*
	meaning that for 16-bit aligned pointer we have 4 least significant bits that are always 0 (for each multiple of 16), which we can use to mark
	whether or not in geometry_hierarchies item contains pointer or gpu_id.

	so in short we have a 64-bit fake pointer that:
		-> LSB (aka bit 0) -> single mesh marker,
		-> sizeof(id::id_type) into bits MSB (aka e.g. bits 63 - 31 if id::id_type is 32-bit value) -> gpu_id
	*/

	// create a fake pointer and put it in the geometry_hierarchies.
	static_assert(sizeof(uintptr_t) > sizeof(id::id_type));
	constexpr u8 shift_bits{ (sizeof(uintptr_t) - sizeof(id::id_type)) << 3 };
	u8* const fake_pointer{ (u8* const)((((uintptr_t)gpu_id) << shift_bits) | single_mesh_marker) };

	std::lock_guard lock{ geometry_mutex };
	return geometry_hierarchies.add(fake_pointer);
}

// Determine whether or not this geometry has a single lod with a single submesh.
// NOTE: expects the same data as create_geometry_resource()
bool
is_single_mesh(const void* const data) {
	assert(data);
	utl::blob_stream_reader blob{ (const u8*)data };
	const u32 lod_count{ blob.read<u32>() };
	assert(lod_count);
	if (lod_count > 1) return false;

	blob.skip(sizeof(f32)); // skip over threshold.
	const u32 submesh_count{ blob.read<u32>() };
	assert(submesh_count);
	return submesh_count == 1;
}

id::id_type
gpu_id_from_fake_pointer(u8* const ptr) {
	assert((uintptr_t)ptr & single_mesh_marker);
	static_assert(sizeof(uintptr_t) > sizeof(id::id_type));

	constexpr u8 shift_bits{ (sizeof(uintptr_t) - sizeof(id::id_type)) << 3 };
	// id::invalid_id part is to clear the higher bits. Not necessary but nice for debugging I suppose.
	return (((uintptr_t)ptr) >> shift_bits) & (uintptr_t)id::invalid_id;
}

// NOTE: Expects 'data' to contain: 
/*
* struct {
*	u32 lod_count;
	struct {
		f32 lod_treshold,
		u32 submesh_count,
		u32 size_of_submeshes,
		struct {
			u32 element_size, u32 vertex_count,
			u32 index_count, u32 elements_type, u32 primitive_topology,
			u8 positions[sizeof(f32) * 3 * vertex_count], // sizeof(positions) must be a multiple of 4 bytes.
														  // Pad if needed.
			u8 elements[sizeof(element_size) * vertex_count], // sizeof(elements) must be a multiple of 4 bytes.
															  // Pad if needed.
			u8 indices[index_size * index_count]
		} submeshes[submesh_count]
	} mesh_lods[lod_count]
} geometry;


Output format: 

If geometry has more than one LOD or submesh:
struct {
	u32 lod_count,
	f32 tresholds[lod_count],
	struct {
		u16 offset,
		u16 count
	} lod_offsets[lod_count],
	id::id_type gpu_ids[total_number_of_submeshes]
} geometry_hierarchy;

If geometry has a single LOD or submesh:
(gpu_id << 32) | 0x01
*/
id::id_type 
create_geometry_resource(const void* const data) {
	assert(data);
	return is_single_mesh(data) ? create_single_mesh(data) : create_mesh_hierarchy(data);
}

void 
destroy_geometry_resource(id::id_type id) {
	std::lock_guard lock{ geometry_mutex };
	u8* const pointer{ geometry_hierarchies[id] };
	if ((uintptr_t)pointer & single_mesh_marker) {
		gfx::remove_submesh(gpu_id_from_fake_pointer(pointer));
	}
	else {
		geometry_hierarchy_stream stream{ pointer };
		const u32 lod_count{ stream.lod_count() };
		u32 id_index{ 0 };
		for (u32 lod{ 0 }; lod < lod_count; ++lod) {
			for (u32 i{ 0 }; i < stream.lod_offsets()[lod].count; ++i) {
				gfx::remove_submesh(stream.gpu_ids()[id_index++]);
			}
		}

		free(pointer);
	}
	geometry_hierarchies.remove(id);
}

} // anonymous namespace

id::id_type 
create_resource(const void* const data, asset_type::type type) {
    assert(data);
    id::id_type id{ id::invalid_id };

	switch (type) {
		case mage::content::asset_type::animation: break;
		case mage::content::asset_type::audio: break;
		case mage::content::asset_type::material: break;
		case mage::content::asset_type::mesh: id = create_geometry_resource(data); break;
		case mage::content::asset_type::skeleton: break;
		case mage::content::asset_type::texture: break;
	}

	assert(id::is_valid(id));
	return id;
}

void 
destroy_resource(id::id_type id, asset_type::type type) {
	assert(id::is_valid(id));
	switch (type) {
		case mage::content::asset_type::animation: break;
		case mage::content::asset_type::audio: break;
		case mage::content::asset_type::material: break;
		case mage::content::asset_type::mesh: destroy_geometry_resource(id); break;
		case mage::content::asset_type::skeleton: break;
		case mage::content::asset_type::texture: break;
		default:
			assert(false);
			break;
	}
}


} // namespace mage::content