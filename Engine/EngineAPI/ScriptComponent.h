#pragma once

#include "../Components/ComponentsCommon.h"

namespace mage::script {
    // in lower case because script_id undercover is just an 
    // id::id_type which is u32 for now. Can be diffrent unsigned integer type
    DEFINE_TYPED_ID(script_id);

    class Component final {
    public:
        constexpr explicit Component(script_id id) : _id{ id } {}
        constexpr Component() : _id{ id::invalid_id } {}

        constexpr script_id get_id() { return _id; }
        constexpr bool is_valid() const { return id::is_valid(_id); }

    private:
        script_id _id;
    };
}