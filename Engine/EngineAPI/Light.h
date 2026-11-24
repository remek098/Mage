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
    void attenuation(math::vec3 attenuation) const;
    // where the light's intensity approaches 0 after applying the falloff function
    void range(f32 range) const;
    // umbra -> cone where the light's intensity doesn't depend on the angle with the light's direction vector.
    // penumbra -> cone where the light's intensity gets weaker the more we deviate from the light's direction vector
    // both must be (0, 180) degrees and the umbra can never be greater than penumbra
    void cone_angles(f32 umbra, f32 penumbra) const; 

    bool is_enabled() const;
    f32 intensity() const;
    math::vec3 color() const;
    math::vec3 attenuation() const;
    f32 range() const;
    f32 umbra() const;
    f32 penumbra() const;


    type light_type() const;
    id::id_type entity_id() const;
private:
    u64         _light_set_key{ 0 }; // indicates to which light set the light belongs
    light_id    _id{ id::invalid_id };

};

}