#include "D3D12Light.h"
#include "D3D12Core.h"
#include "Shaders/SharedTypes.h"
#include "EngineAPI/GameEntity.h"
#include "Components/Transform.h"

namespace mage::gfx::d3d12::light {

namespace {

template<u32 n>
struct u32_set_bits {
    static_assert(n > 0 && n <= 32);
    constexpr static const u32 bits{ u32_set_bits<n - 1>::bits | (1 << (n - 1)) };
};

// stops recursiveness of our template
template<>
struct u32_set_bits<0> {
    constexpr static const u32 bits{ 0 };
};

static_assert(u32_set_bits<frame_buffer_count>::bits < (1 << 8), "That's quite a large frame buffer count bro!");
// just u32 int with as many bits set as there's frame buffers.

constexpr u8 dirty_bits_mask{ (u8)u32_set_bits<frame_buffer_count>::bits };

struct light_owner {
    game_entity::entity_id  entity_id{ id::invalid_id };
    u32                     data_index{ u32_invalid_id }; // pointing into tightly packed array in gpu
    gfx::light::type        type;
    bool                    is_enabled;
};

// because we cannot call std::vector::size() inside constexpr function.
#if USE_STL_VECTOR
#define CONSTEXPR
#else
#define CONSTEXPR constexpr
#endif

class light_set {
public:
    constexpr gfx::light add(const light_init_info& info) {
        if (info.type == gfx::light::directional) {
            // NOTE: not going to bother to pack directional lights since number of them should be low.
            // normally only sun and the moon.

            // find an available slot in the array if any.
            u32 index{ u32_invalid_id };
            for (u32 i{ 0 }; i < _non_cullable_owners.size(); ++i) {
                if (!id::is_valid(_non_cullable_owners[i])) {
                    index = i;
                    break;
                }
            }

            // if we found invalid index, emplace new directional light
            if (index == u32_invalid_id) {
                index = (u32)_non_cullable_owners.size();
                _non_cullable_owners.emplace_back();
                _non_cullable_lights.emplace_back();
            } // index is either the index of found directional light or the index of newly created directional light

            hlsl::DirectionalLightParameters& params{ _non_cullable_lights[index] };
            params.Color = info.color;
            params.Intensity = info.intensity;

            light_owner owner{ game_entity::entity_id{info.entity_id}, index, info.type, info.is_enabled };
            const light_id id{ _owners.add(owner) };
            _non_cullable_owners[index] = id;

            return gfx::light{ id, info.light_set_key };
        }
        else {
            u32 index{ u32_invalid_id };

            // look for the empty spot after the enabled lights
            for (u32 i{ _enabled_light_count }; i < _cullable_owners.size(); ++i) {
                if (!id::is_valid(_cullable_owners[i])) {
                    index = i;
                    break;
                }
            }

            // if there was no empty slot found, add a new item.
            if (index == u32_invalid_id) {
                index = (u32)_cullable_owners.size();
                _cullable_lights.emplace_back();
                _culling_infos.emplace_back();
                _cullable_entity_ids.emplace_back();
                _cullable_owners.emplace_back();
                _dirty_bits.emplace_back();
                assert(_cullable_owners.size() == _cullable_lights.size());
                assert(_cullable_owners.size() == _culling_infos.size());
                assert(_cullable_owners.size() == _cullable_entity_ids.size());
                assert(_cullable_owners.size() == _dirty_bits.size());
            }

            add_cullable_light_parameters(info, index);
            add_light_culling_info(info, index);
            const light_id id{ _owners.add(light_owner{game_entity::entity_id{info.entity_id}, index, info.type, info.is_enabled}) };
            _cullable_entity_ids[index] = _owners[id].entity_id;
            _cullable_owners[index] = id;
            _dirty_bits[index] = dirty_bits_mask;
            enable(id, info.is_enabled);
            update_transform(index);

            return gfx::light{ id, info.light_set_key };
        }
    }

    constexpr void remove(light_id id) {
        enable(id, false); // disable light

        const light_owner& owner{ _owners[id] };

        if (owner.type == gfx::light::directional) {
            _non_cullable_owners[owner.data_index] = light_id{ id::invalid_id }; // that's how available slot is marked for removed lights
        }
        else {
            assert(_owners[_cullable_owners[owner.data_index]].data_index == owner.data_index);
            _cullable_owners[owner.data_index] = light_id{ id::invalid_id };
        }

        _owners.remove(id);
    }

    void update_transforms() {
        // update direction for non-cullable lights
        for (const auto& id : _non_cullable_owners) {
            if (!id::is_valid(id)) continue;

            const light_owner& owner{ _owners[id] };
            const game_entity::entity entity{ game_entity::entity_id{owner.entity_id} };
            hlsl::DirectionalLightParameters& params{ _non_cullable_lights[owner.data_index] };
            params.Direction = entity.orientation();
        }

        const u32 count{ _enabled_light_count };
        if (!count) return;

        // update position and direction of cullable lights
        assert(_cullable_entity_ids.size() >= count);
        _transform_flags_cache.resize(count);
        transform::get_updated_components_flags(_cullable_entity_ids.data(), count, _transform_flags_cache.data());

        for (u32 i{ 0 }; i < count; ++i) {
            // NOTE: we don't care about checking transform cache flags since we will be updating position and direction in light buffer anyway.
            if (_transform_flags_cache[i]) {
                update_transform(i);
            }
        }
    }

    constexpr void enable(light_id id, bool is_enabled) {
        _owners[id].is_enabled = is_enabled;
        if (_owners[id].type == gfx::light::directional) {
            return;
        }
        // cullable lights
        const u32 data_index{ _owners[id].data_index };

        // NOTE: this is a reference to _enabled_light_count and will change it's value!
        u32& count{ _enabled_light_count };

        // handle 2 cases, where user wants the lights to be either enabled or disabled.
        // NOTE: dirty_bits is going to be set by swap_cullable_lights() so we don't set it here.
        if (is_enabled) {
            if (data_index > count) {
                assert(count < _cullable_lights.size());
                swap_cullable_lights(data_index, count);
                ++count;
            }
            else if (data_index == count) {
                // if it was the first disabled light, we just increment the _enabled_light_count there.
                ++count;
            }
        }
        else if (count > 0) {
            const u32 last{ count - 1 };
            if (data_index < last) {
                // if the light that we want to disable is not the last enabled light, we swap it with last enabled light 
                // and then decrement the _enabled_light_count
                swap_cullable_lights(data_index, last);
                --count;
            }
            else if (data_index == last) {
                --count;
            }
        }
    }

    constexpr void intensity(light_id id, f32 intensity) {
        if (intensity < 0.f) intensity = 0.f;

        const light_owner& owner{ _owners[id] };
        const u32 index{ owner.data_index };

        if (owner.type == gfx::light::directional) {
            assert(index < _non_cullable_lights.size());
            _non_cullable_lights[index].Intensity = intensity;
        }
        else {
            assert(_owners[_cullable_owners[index]].data_index == index);
            assert(index < _cullable_lights.size());

            _cullable_lights[index].Intensity = intensity;
            _dirty_bits[index] = dirty_bits_mask; // set dirty flag so that gpu buffers will be updated.
        }
    }

    constexpr void color(light_id id, math::vec3 color) {
        assert(color.x <= 1.f && color.y <= 1.f && color.z <= 1.f);
        assert(color.x >= 0.f && color.y >= 0.f && color.z >= 0.f);

        const light_owner& owner{ _owners[id] };
        const u32 index{ owner.data_index };

        if (owner.type == gfx::light::directional) {
            assert(index < _non_cullable_lights.size());
            _non_cullable_lights[index].Color = color;
        }
        else {
            assert(_owners[_cullable_owners[index]].data_index == index);
            assert(index < _cullable_lights.size());

            _cullable_lights[index].Color = color;
            _dirty_bits[index] = dirty_bits_mask; // set dirty flag so that gpu buffers will be updated.
        }
    }

    CONSTEXPR void attenuation(light_id id, math::vec3 attenuation) {
        assert(attenuation.x >= 0.f && attenuation.y >= 0.f && attenuation.z >= 0.f);
        const light_owner& owner{ _owners[id] };
        const u32 index{ owner.data_index };

        assert(owner.type != gfx::light::directional);
        assert(_owners[_cullable_owners[index]].data_index == index);
        assert(index < _cullable_lights.size());

        _cullable_lights[index].Attenuation = attenuation;
        _dirty_bits[index] = dirty_bits_mask; // set dirty flag so that gpu buffers will be updated.
    }

    CONSTEXPR void range(light_id id, f32 range) {
        assert(range > 0.f);
        const light_owner& owner{ _owners[id] };
        const u32 index{ owner.data_index };

        assert(owner.type != gfx::light::directional);
        assert(_owners[_cullable_owners[index]].data_index == index);
        assert(index < _cullable_lights.size());

        _cullable_lights[index].Range = range;
        _culling_infos[index].Range = range;
        _dirty_bits[index] = dirty_bits_mask; // set dirty flag so that gpu buffers will be updated.

        if (owner.type == gfx::light::spot) {
            _culling_infos[index].ConeRadius = calculate_cone_radius(range, _cullable_lights[index].CosPenumbra);
        }
    }

    void umbra(light_id id, f32 umbra) {
        const light_owner& owner{ _owners[id] };
        const u32 index{ owner.data_index };

        assert(owner.type == gfx::light::spot);
        assert(_owners[_cullable_owners[index]].data_index == index);
        assert(index < _cullable_lights.size());

        umbra = math::clamp(umbra, 0.f, math::pi);
        _cullable_lights[index].CosUmbra = DirectX::XMScalarCos(umbra * 0.5f);
        _dirty_bits[index] = dirty_bits_mask; // set dirty flag so that gpu buffers will be updated.

        if (penumbra(id) < umbra) {
            penumbra(id, umbra);
        }
    }

    void penumbra(light_id id, f32 penumbra) {
        const light_owner& owner{ _owners[id] };
        const u32 index{ owner.data_index };

        assert(owner.type == gfx::light::spot);
        assert(_owners[_cullable_owners[index]].data_index == index);
        assert(index < _cullable_lights.size());

        penumbra = math::clamp(penumbra, umbra(id), math::pi);
        _cullable_lights[index].CosPenumbra = DirectX::XMScalarCos(penumbra * 0.5f);
        
        _culling_infos[index].ConeRadius = calculate_cone_radius(range(id), _cullable_lights[index].CosPenumbra);
        _dirty_bits[index] = dirty_bits_mask; // set dirty flag so that gpu buffers will be updated.
    }

    constexpr bool is_enabled(light_id id) const {
        return _owners[id].is_enabled;
    }

    constexpr f32 intensity(light_id id) const {
        const light_owner& owner{ _owners[id] };
        const u32 index{ owner.data_index };

        if (owner.type == gfx::light::directional) {
            assert(index < _non_cullable_lights.size());
            return _non_cullable_lights[index].Intensity;
        }

        assert(_owners[_cullable_owners[index]].data_index == index);
        assert(index < _cullable_lights.size());
        return _cullable_lights[index].Intensity;
    }

    constexpr math::vec3 color(light_id id) {
        const light_owner& owner{ _owners[id] };
        const u32 index{ owner.data_index };

        if (owner.type == gfx::light::directional) {
            assert(index < _non_cullable_lights.size());
            return _non_cullable_lights[index].Color;
        }
        
        assert(_owners[_cullable_owners[index]].data_index == index);
        assert(index < _cullable_lights.size());
        return _cullable_lights[index].Color;
    }

    CONSTEXPR math::vec3 attenuation(light_id id) const {
        const light_owner& owner{ _owners[id] };
        const u32 index{ owner.data_index };
        assert(owner.type != gfx::light::directional);
        assert(_owners[_cullable_owners[index]].data_index == index);
        assert(index < _cullable_lights.size());
        return _cullable_lights[index].Attenuation;
    }

    CONSTEXPR f32 range(light_id id) const {
        const light_owner& owner{ _owners[id] };
        const u32 index{ owner.data_index };
        assert(owner.type != gfx::light::directional);
        assert(_owners[_cullable_owners[index]].data_index == index);
        assert(index < _cullable_lights.size());
        return _cullable_lights[index].Range;
    }

    f32 umbra(light_id id) const {
        const light_owner& owner{ _owners[id] };
        const u32 index{ owner.data_index };
        assert(owner.type == gfx::light::spot);
        assert(_owners[_cullable_owners[index]].data_index == index);
        assert(index < _cullable_lights.size());
        return DirectX::XMScalarACos(_cullable_lights[index].CosUmbra) * 2.f; // calculating from half angle
    }

    f32 penumbra(light_id id) const {
        const light_owner& owner{ _owners[id] };
        const u32 index{ owner.data_index };
        assert(owner.type == gfx::light::spot);
        assert(_owners[_cullable_owners[index]].data_index == index);
        assert(index < _cullable_lights.size());
        return DirectX::XMScalarACos(_cullable_lights[index].CosPenumbra) * 2.f; // calculating from half angle
    }

    constexpr gfx::light::type type(light_id id) const {
        return _owners[id].type;
    }

    constexpr id::id_type entity_id(light_id id) const {
        return _owners[id].entity_id;
    }

    // return the number of enabled directional lights
    CONSTEXPR u32 non_cullable_lights_count() const {
        u32 count{ 0 };
        for (const auto& id : _non_cullable_owners) {
            if (id::is_valid(id) && _owners[id].is_enabled) ++count;
        }
        return count;
    }

    // copy a non cullable light data into the buffer
    CONSTEXPR void non_cullable_lights(hlsl::DirectionalLightParameters* const lights, [[maybe_unused]] u32 buffer_size) {
        // make sure size has proper alignment for D3D12 Constant Buffer.
        assert(buffer_size == math::align_size_up<D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT>(
            non_cullable_lights_count() * sizeof(hlsl::DirectionalLightParameters)));

        const u32 count{ (u32)_non_cullable_owners.size() };
        u32 index{ 0 };
        for (u32 i{ 0 }; i < count; ++i) {
            // because array is not tightly packed, skip the ones that don't have proper data
            if (!id::is_valid(_non_cullable_owners[i])) continue;

            const light_owner& owner{ _owners[_non_cullable_owners[i]] };
            if (owner.is_enabled) {
                assert(_owners[_non_cullable_owners[i]].data_index == i);
                lights[index] = _non_cullable_lights[i];
                ++index;
            }
        }
    }

    constexpr u32 cullable_lights_count() const { return _enabled_light_count; }
    constexpr bool has_lights() const { return _owners.size() > 0; }

private:
    f32 calculate_cone_radius(f32 range, f32 cos_penumbra) {
        const f32 sin_penumbra{ sqrt(1.f - cos_penumbra * cos_penumbra) };

        //if (cos_penumbra >= 0.707107f) {
        //    // if penumbra half angle is less than 45 degrees
        //    return sin_penumbra * range / cos_penumbra;
        //}
        //else {
        //    return sin_penumbra * range;
        //}
        return sin_penumbra * range;
    }

    void update_transform(u32 index) {
        const game_entity::entity entity{ game_entity::entity_id{_cullable_entity_ids[index]} };
        hlsl::LightParameters& params{ _cullable_lights[index] };
        params.Position = entity.position();

        hlsl::LightCullingLightInfo& culling_info{ _culling_infos[index] };
        culling_info.Position = entity.position();

        if (params.Type == gfx::light::spot) {
            culling_info.Direction = params.Direction = entity.orientation();
        }
        _dirty_bits[index] = dirty_bits_mask; // for all frame buffers so that this change will be reflected in all gpu buffers.
    }

    CONSTEXPR void add_cullable_light_parameters(const light_init_info& info, u32 index) {
        using gfx::light;
        assert(info.type != light::directional && index < _cullable_lights.size());

        hlsl::LightParameters& params{ _cullable_lights[index] };
        params.Type = info.type;
        assert(params.Type < light::count);
        params.Color = info.color;
        params.Intensity = info.intensity;

        if (params.Type == light::point) {
            const point_light_params& p{ info.point_params };
            params.Attenuation = p.attenuation;
            params.Range = p.range;
        }
        else if (params.Type == light::spot) {
            const spot_light_params& p{ info.spot_params };
            params.Attenuation = p.attenuation;
            params.Range = p.range;
            params.CosUmbra = DirectX::XMScalarCos(p.umbra * 0.5f);
            params.CosPenumbra = DirectX::XMScalarCos(p.penumbra * 0.5f);
        }
    }

    CONSTEXPR void add_light_culling_info(const light_init_info& info, u32 index) {
        using gfx::light;
        assert(info.type != light::directional && index < _culling_infos.size());

        // these should be initialized at this point
        hlsl::LightParameters& params{ _cullable_lights[index] };
        assert(params.Type == info.type);

        hlsl::LightCullingLightInfo& culling_info{ _culling_infos[index] };
        culling_info.Range = params.Range;
        culling_info.Type = params.Type;

        if (info.type == light::spot) {
            culling_info.ConeRadius = calculate_cone_radius(params.Range, params.CosPenumbra);
        }
    }

    void swap_cullable_lights(u32 index1, u32 index2) {
        assert(index1 != index2);

        // swap light parameter indices
        assert(index1 < _cullable_owners.size());
        assert(index2 < _cullable_owners.size());
        assert(id::is_valid(_cullable_owners[index1]));
        assert(id::is_valid(_cullable_owners[index2]));
        assert(index1 < _cullable_lights.size());
        assert(index2 < _cullable_lights.size());
        assert(index1 < _culling_infos.size());
        assert(index2 < _culling_infos.size());
        assert(index1 < _cullable_entity_ids.size());
        assert(index2 < _cullable_entity_ids.size());
        // light might be enabled/disabled, but cannot be a removed light.
        assert(id::is_valid(_cullable_owners[index1]) || id::is_valid(_cullable_owners[index2]));

        // if one of the lights is a removed light, we make sure it's index is always held by index1 variable.
        if (!id::is_valid(_cullable_owners[index2])) {
            std::swap(index1, index2);
        }

        // handle the case where index1 was the removed light.
        if (!id::is_valid(_cullable_owners[index1])) {
            light_owner& owner2{ _owners[_cullable_owners[index2]] };
            assert(owner2.data_index == index2);
            owner2.data_index = index1;

            _cullable_lights[index1] = _cullable_lights[index2];
            _culling_infos[index1] = _culling_infos[index2];
            _cullable_entity_ids[index1] = _cullable_entity_ids[index2];
            std::swap(_cullable_owners[index1], _cullable_owners[index2]);
            _dirty_bits[index1] = dirty_bits_mask;
            assert(_owners[_cullable_owners[index1]].entity_id == _cullable_entity_ids[index1]);
            id::is_valid(_cullable_owners[index2]);
        }
        else { // handle  the case where both lights are valid
            // NOTE: remember that light_owners are in the free_list and will stay in the same slot until they're removed.
            //       the light data however is moved around to keep the array contiguous.
            light_owner& owner1{ _owners[_cullable_owners[index1]] };
            light_owner& owner2{ _owners[_cullable_owners[index2]] };
            assert(owner1.data_index == index1);
            assert(owner2.data_index == index2);
            owner1.data_index = index2;
            owner2.data_index = index1;

            std::swap(_cullable_lights[index1], _cullable_lights[index2]);
            std::swap(_culling_infos[index1], _culling_infos[index2]);
            std::swap(_cullable_entity_ids[index1], _cullable_entity_ids[index2]);
            std::swap(_cullable_owners[index1], _cullable_owners[index2]);

            // make sure that all indices are consistent.
            assert(_owners[_cullable_owners[index1]].entity_id == _cullable_entity_ids[index1]);
            assert(_owners[_cullable_owners[index2]].entity_id == _cullable_entity_ids[index2]);

            // set dirty bits
            assert(index1 < _dirty_bits.size());
            assert(index2 < _dirty_bits.size());
            _dirty_bits[index1] = dirty_bits_mask;
            _dirty_bits[index2] = dirty_bits_mask;
        }
    }
    
private:
    // NOTE: these are not tightly packed.
    utl::free_list<light_owner>                     _owners;
    utl::vector<hlsl::DirectionalLightParameters>   _non_cullable_lights;
    utl::vector<light_id>                           _non_cullable_owners; // contains index to each owner

    // NOTE: these are tightly packed.
    utl::vector<hlsl::LightParameters>              _cullable_lights;
    utl::vector<hlsl::LightCullingLightInfo>        _culling_infos;
    utl::vector<game_entity::entity_id>             _cullable_entity_ids;
    utl::vector<light_id>                           _cullable_owners; // contains index to each owner
    // if we change any of light properties (e.g. position and direction)
    // then we need to update this data and gpu buffers as well. That's why we have this _dirty_bits array to indicate that.
    // Elements in this array have bits set for each frame buffer that needs to be updated with light data.
    utl::vector<u8>                                 _dirty_bits;

    utl::vector<u8>                                 _transform_flags_cache;
    u32                                             _enabled_light_count{ 0 }; // number of cullable lights

    friend class d3d12_light_buffer;
};

class d3d12_light_buffer {
public:
    d3d12_light_buffer() = default;
    CONSTEXPR void update_light_buffers(light_set& set, u64 light_set_key, u32 frame_index) {
        // buffer sizes needed to contain our light data
        u32 sizes[light_buffer::count]{};
        sizes[light_buffer::non_cullable_light] = set.non_cullable_lights_count() * sizeof(hlsl::DirectionalLightParameters);
        sizes[light_buffer::cullable_light] = set.cullable_lights_count() * sizeof(hlsl::LightParameters);
        sizes[light_buffer::culling_info] = set.cullable_lights_count() * sizeof(hlsl::LightCullingLightInfo);

        // get current buffer size and check if we need to resize it.
        u32 current_sizes[light_buffer::count]{};
        current_sizes[light_buffer::non_cullable_light] = _buffers[light_buffer::non_cullable_light].buffer.size();
        current_sizes[light_buffer::cullable_light] = _buffers[light_buffer::cullable_light].buffer.size();
        current_sizes[light_buffer::culling_info] = _buffers[light_buffer::culling_info].buffer.size();

        if (current_sizes[light_buffer::non_cullable_light] < sizes[light_buffer::non_cullable_light]) {
            resize_buffer(light_buffer::non_cullable_light, sizes[light_buffer::non_cullable_light], frame_index);
        }

        // this member function will make sure the data is copied to gpu buffer.
        set.non_cullable_lights((hlsl::DirectionalLightParameters *const)_buffers[light_buffer::non_cullable_light].cpu_address,
                                _buffers[light_buffer::non_cullable_light].buffer.size());

        // update cullable lights buffers
        bool buffers_resized{ false };
        if (current_sizes[light_buffer::cullable_light] < sizes[light_buffer::cullable_light]) {
            assert(current_sizes[light_buffer::culling_info] < sizes[light_buffer::culling_info]);
            resize_buffer(light_buffer::cullable_light, sizes[light_buffer::cullable_light], frame_index);
            resize_buffer(light_buffer::culling_info, sizes[light_buffer::culling_info], frame_index);
            buffers_resized = true;
        }

        bool all_lights_updated{ false };
        if (buffers_resized || _current_light_set_key != light_set_key) {
            // copy entire light data buffer.
            memcpy(_buffers[light_buffer::cullable_light].cpu_address, set._cullable_lights.data(), sizes[light_buffer::cullable_light]);
            memcpy(_buffers[light_buffer::culling_info].cpu_address, set._culling_infos.data(), sizes[light_buffer::culling_info]);
            _current_light_set_key = light_set_key;
            all_lights_updated = true;
        }

        assert(_current_light_set_key == light_set_key);
        const u32 index_mask{ 1UL << frame_index };
        
        if (all_lights_updated) {
            // not a single dirty mask matters, we clear all flags for current frame index.
            for (u32 i{ 0 }; i < set.cullable_lights_count(); ++i) {
                set._dirty_bits[i] &= ~index_mask;
            }
        }
        else {
            for (u32 i{ 0 }; i < set.cullable_lights_count(); ++i) {
                if (set._dirty_bits[i] & index_mask) {
                    assert(i * sizeof(hlsl::LightParameters) < sizes[light_buffer::cullable_light]);
                    assert(i * sizeof(hlsl::LightCullingLightInfo) < sizes[light_buffer::culling_info]);
                    u8 *const light_dst{ _buffers[light_buffer::cullable_light].cpu_address + (i * sizeof(hlsl::LightParameters)) };
                    u8 *const culling_dst{ _buffers[light_buffer::culling_info].cpu_address + (i * sizeof(hlsl::LightCullingLightInfo)) };
                    memcpy(light_dst, &set._cullable_lights[i], sizeof(hlsl::LightParameters));
                    memcpy(culling_dst, &set._culling_infos[i], sizeof(hlsl::LightCullingLightInfo));
                    set._dirty_bits[i] &= ~index_mask;

                }
            }
        }
    }

    constexpr void release() {
        for (u32 i{ 0 }; i < light_buffer::count; ++i) {
            _buffers[i].buffer.release();
            _buffers[i].cpu_address = nullptr;
        }
    }

    constexpr D3D12_GPU_VIRTUAL_ADDRESS non_cullable_lights() const { return _buffers[light_buffer::non_cullable_light].buffer.gpu_address(); }
    constexpr D3D12_GPU_VIRTUAL_ADDRESS cullable_lights() const { return _buffers[light_buffer::cullable_light].buffer.gpu_address(); }
    constexpr D3D12_GPU_VIRTUAL_ADDRESS culling_infos() const { return _buffers[light_buffer::culling_info].buffer.gpu_address(); }

private:
    struct light_buffer {
        enum type : u32 {
            non_cullable_light,
            cullable_light,
            culling_info,

            count
        };

        d3d12_buffer        buffer{};
        u8*                 cpu_address{ nullptr };
    };

    void resize_buffer(light_buffer::type type, u32 size, [[maybe_unused]] u32 frame_index) {
        assert(type < light_buffer::count);
        if (!size) return;

        _buffers[type].buffer.release();
        _buffers[type].buffer = d3d12_buffer{ constant_buffer::get_default_init_info(size), true };
        NAME_D3D12_OBJECT_INDEXED(_buffers[type].buffer.buffer(), frame_index,
                                  type == light_buffer::non_cullable_light ? L"Non-cullable Light Buffer" :
                                  type == light_buffer::cullable_light ? L"Cullable Light Buffer" : L"Light Culling Info Buffer");

        // write the memory address of mapped memory to our cpu_address variable.
        D3D12_RANGE range{};
        DXCALL(_buffers[type].buffer.buffer()->Map(0, &range, (void **)(&_buffers[type].cpu_address)));
        assert(_buffers[type].cpu_address); // check that it's not a nullptr
    }

private:
    light_buffer            _buffers[light_buffer::count];
    u64                     _current_light_set_key{ 0 };
};



std::unordered_map<u64, light_set>      light_sets;
d3d12_light_buffer                      light_buffers[frame_buffer_count];


constexpr void
set_is_enabled(light_set& set, light_id id, const void *const data, [[maybe_unused]] u32 size) {
    bool is_enabled{ *(bool*)data };
    assert(sizeof(is_enabled) == size);
    set.enable(id, is_enabled);
}

constexpr void
set_intensity(light_set& set, light_id id, const void *const data, [[maybe_unused]] u32 size) {
    f32 intensity{ *(f32*)data };
    assert(sizeof(intensity) == size);
    set.intensity(id, intensity);
}

constexpr void
set_color(light_set& set, light_id id, const void *const data, [[maybe_unused]] u32 size) {
    math::vec3 color{ *(math::vec3*)data };
    assert(sizeof(color) == size);
    set.color(id, color);
}

CONSTEXPR void
set_attenuation(light_set& set, light_id id, const void *const data, [[maybe_unused]] u32 size) {
    math::vec3 attenuation{ *(math::vec3*)data };
    assert(sizeof(attenuation) == size);
    set.attenuation(id, attenuation);
}

CONSTEXPR void
set_range(light_set& set, light_id id, const void *const data, [[maybe_unused]] u32 size) {
    f32 range{ *(f32*)data };
    assert(sizeof(range) == size);
    set.range(id, range);
}

void
set_umbra(light_set& set, light_id id, const void *const data, [[maybe_unused]] u32 size) {
    f32 umbra{ *(f32*)data };
    assert(sizeof(umbra) == size);
    set.umbra(id, umbra);
}

void
set_penumbra(light_set& set, light_id id, const void *const data, [[maybe_unused]] u32 size) {
    f32 penumbra{ *(f32*)data };
    assert(sizeof(penumbra) == size);
    set.penumbra(id, penumbra);
}

constexpr void
get_is_enabled(light_set& set, light_id id, void *const data, [[maybe_unused]] u32 size) {
    bool *const is_enabled{ (bool *const)data };
    assert(sizeof(bool) == size);
    *is_enabled = set.is_enabled(id);
}

constexpr void
get_intensity(light_set& set, light_id id, void *const data, [[maybe_unused]] u32 size) {
    f32* const intensity{ (f32 *const)data };
    assert(sizeof(f32) == size);
    *intensity = set.intensity(id);
}

constexpr void
get_color(light_set& set, light_id id, void *const data, [[maybe_unused]] u32 size) {
    math::vec3 *const color{ (math::vec3 *const)data };
    assert(sizeof(math::vec3) == size);
    *color = set.color(id);
}

CONSTEXPR void
get_attenuation(light_set& set, light_id id, void *const data, [[maybe_unused]] u32 size) {
    math::vec3 *const attenuation{ (math::vec3 *const)data };
    assert(sizeof(math::vec3) == size);
    *attenuation = set.attenuation(id);
}

CONSTEXPR void
get_range(light_set& set, light_id id, void *const data, [[maybe_unused]] u32 size) {
    f32* const range{ (f32 *const)data };
    assert(sizeof(f32) == size);
    *range = set.range(id);
}

void
get_umbra(light_set& set, light_id id, void *const data, [[maybe_unused]] u32 size) {
    f32* const umbra{ (f32 *const)data };
    assert(sizeof(f32) == size);
    *umbra = set.umbra(id);
}

void
get_penumbra(light_set& set, light_id id, void *const data, [[maybe_unused]] u32 size) {
    f32* const penumbra{ (f32 *const)data };
    assert(sizeof(f32) == size);
    *penumbra = set.penumbra(id);
}

constexpr void
get_type(light_set &set, light_id id, void *const data, [[maybe_unused]] u32 size) {
    gfx::light::type *const type{ (gfx::light::type *const)data };
    assert(sizeof(gfx::light::type) == size);
    *type = set.type(id);
}

constexpr void
get_entity_id(light_set &set, light_id id, void *const data, [[maybe_unused]] u32 size) {
    id::id_type *const entity_id{ (id::id_type *const)data };
    assert(sizeof(id::id_type) == size);
    *entity_id = set.entity_id(id);
}

constexpr void
dummy_set_fn(light_set&, light_id, const void *const, u32) {}

using set_function = void(*)(light_set&, light_id, const void *const, u32);
using get_function = void(*)(light_set&, light_id, void *const, u32);

constexpr set_function set_functions[]{
    set_is_enabled,
    set_intensity,
    set_color,
    set_attenuation,
    set_range,
    set_umbra,
    set_penumbra,
    dummy_set_fn,
    dummy_set_fn,   // dummy pointers because we have some more params than 3 (for now because right now we want to support directional light)
};
static_assert(_countof(set_functions) == light_parameter::count);


constexpr get_function get_functions[]{
    get_is_enabled,
    get_intensity,
    get_color,
    get_attenuation,
    get_range,
    get_umbra,
    get_penumbra,
    get_type,
    get_entity_id,
};
static_assert(_countof(get_functions) == light_parameter::count);

#undef CONSTEXPR
} // anonymous namespace

bool 
initialize() {
    return true;
}

void 
shutdown() {
    // TODO: make sure to remove all lights before shutting down graphics.
    assert([] {
        bool has_lights{ false };
        for (const auto& it : light_sets) {
            has_lights |= it.second.has_lights();
        }
        return !has_lights;
           }());

    for (u32 i{ 0 }; i < frame_buffer_count; ++i) {
        light_buffers[i].release();
    }
}


gfx::light
create(light_init_info info) {
    assert(id::is_valid(info.entity_id));
    return light_sets[info.light_set_key].add(info);
}

void
remove(light_id id, u64 light_set_key) {
    assert(light_sets.count(light_set_key));
    light_sets[light_set_key].remove(id);
}

void
set_parameter(light_id id, u64 light_set_key, light_parameter::parameter parameter, const void* const data, u32 size) {
    assert(data && size);
    assert(light_sets.count(light_set_key));
    assert(parameter < light_parameter::count && set_functions[parameter] != dummy_set_fn);
    set_functions[parameter](light_sets[light_set_key], id, data, size);
}

void
get_parameter(light_id id, u64 light_set_key, light_parameter::parameter parameter, void* const data, u32 size) {
    assert(data && size);
    assert(light_sets.count(light_set_key));
    assert(parameter < light_parameter::count);
    get_functions[parameter](light_sets[light_set_key], id, data, size);
}

void 
update_light_buffers(const d3d12_frame_info& d3d12_info) {
    const u64 light_set_key{ d3d12_info.info->light_set_key };
    assert(light_sets.count(light_set_key)); // make sure light set with this key already exists.
    light_set& set{ light_sets[light_set_key] };
    if (!set.has_lights()) return;

    // if there're lights in the set, we obtain reference to light_buffer which can be updated using the current light_set
    set.update_transforms(); // update light data first
    const u32 frame_index{ d3d12_info.frame_index };
    d3d12_light_buffer& light_buffer{ light_buffers[frame_index] };
    light_buffer.update_light_buffers(set, light_set_key, frame_index);
}

D3D12_GPU_VIRTUAL_ADDRESS 
non_cullable_light_buffer(u32 frame_index) {
    const d3d12_light_buffer& light_buffer{ light_buffers[frame_index] };
    return light_buffer.non_cullable_lights();
}

u32 
non_cullable_light_count(u64 light_set_key) {
    assert(light_sets.count(light_set_key)); // make sure light set with this key already exists.
    return light_sets[light_set_key].non_cullable_lights_count();
}

} // namespace mage::gfx::d3d12::light