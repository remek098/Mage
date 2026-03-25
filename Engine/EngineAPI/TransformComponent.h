#pragma once

#include "../Components/ComponentsCommon.h"

namespace mage::transform {
    // in lower case because transform_id undercover is just an 
    // id::id_type which is u32 for now. Can be diffrent unsigned integer type
    DEFINE_TYPED_ID(transform_id);

    class Component final {
    public:
        constexpr explicit Component(transform_id id) : _id{ id } {}
        constexpr Component() : _id{ id::invalid_id } {}

        constexpr transform_id get_id() { return _id; }
        constexpr bool is_valid() const { return id::is_valid(_id); }


        math::vec4 rotation() const;
        math::vec3 position() const;
        math::vec3 scale() const;
    private:
        transform_id _id;
    };
}