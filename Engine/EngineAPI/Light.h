#pragma once

#include "CommonHeaders.h"

namespace mage::gfx {

DEFINE_TYPED_ID(light_id);

class light {
public:
    enum type : u32 {
        directional,
        point,
        spot,

        count
    };

    constexpr explicit light(light_id id, u64 light_set_key) : _light_set_key{ light_set_key }, _id { id } {}
    constexpr light() = default;

    constexpr light_id get_id() { return _id; }
    constexpr u64 light_set_key() const { return _light_set_key; }
    constexpr bool is_valid() const { return id::is_valid(_id); }

    void is_enabled(bool is_enabled) const;
    void intensity(f32 intensity) const;
    void color(math::vec3 color) const;

    bool is_enabled() const;
    f32 intensity() const;
    math::vec3 color() const;

    type light_type() const;
    id::id_type entity_id() const;
private:
    u64         _light_set_key{ 0 }; // indicates to which light set the light belongs
    light_id    _id{ id::invalid_id };

};

}