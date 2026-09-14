#include "Transform.h"
#include "Entity.h"

namespace mage::transform {

namespace {
utl::vector<math::vec4> rotations;
utl::vector<math::vec3> orientations;
utl::vector<math::vec3> positions;
utl::vector<math::vec3> scales;

math::vec3
calculate_orientation(math::vec4 rotation) {
    using namespace DirectX;
    XMVECTOR rotation_quat{ XMLoadFloat4(&rotation) };
    XMVECTOR front{ XMVectorSet(0.f, 0.f, 1.f, 0.f) };
    math::vec3 orientation;
    XMStoreFloat3(&orientation, XMVector3Rotate(front, rotation_quat));
    return orientation;
}

} // anonymous namespace

component create(const init_info& info, game_entity::entity entity) {
    assert(entity.is_valid());
    const id::id_type entity_index{ id::index(entity.get_id()) };


    if ( positions.size() > entity_index ) {
        // when entity_index is pointing inside our array, we fill one of holes in array
        math::vec4 rotation{ info.rotation };
        rotations[entity_index] = rotation;
        orientations[entity_index] = calculate_orientation(rotation);
        positions[entity_index] = math::vec3{ info.position };
        scales[entity_index] = math::vec3{ info.scale };
    }
    else {
        assert(positions.size() == entity_index);
        rotations.emplace_back(info.rotation);
        orientations.emplace_back(calculate_orientation(math::vec4{ info.rotation }));
        positions.emplace_back(info.position);
        scales.emplace_back(info.scale);
    }

    return component(transform_id{ entity.get_id() });
}


void remove([[maybe_unused]] component comp) {
    // NOTE: technically we don't have to clean up positions, rotations and scales utl::vectors there,
    // since create() manages allocation quite well.
    assert(comp.is_valid());
}

math::vec3 
component::orientation() const {
    assert(is_valid());
    return orientations[id::index(_id)];
}

math::vec4 component::rotation() const {
    assert(is_valid());
    return rotations[id::index(_id)];
}
    
math::vec3 component::position() const {
    assert(is_valid());
    return positions[id::index(_id)];
}
    
math::vec3 component::scale() const {
    assert(is_valid());
    return scales[id::index(_id)];
}

} // namespace mage::transform