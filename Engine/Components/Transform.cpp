#include "Transform.h"
#include "Entity.h"

namespace mage::transform {

namespace {
utl::vector<math::mat4x4>   to_world;
utl::vector<math::mat4x4>   inv_world;
utl::vector<math::vec4>     rotations;
utl::vector<math::vec3>     orientations;
utl::vector<math::vec3>     positions;
utl::vector<math::vec3>     scales;
utl::vector<u8>             has_transforms; // to find out which entities have their transform matrices already calculated


void
calculate_transform_matrices(id::id_type index) {
    assert(rotations.size() >= index);
    assert(positions.size() >= index);
    assert(scales.size() >= index);

    using namespace DirectX;
    XMVECTOR r{ XMLoadFloat4(&rotations[index]) };
    XMVECTOR t{ XMLoadFloat3(&positions[index]) };
    XMVECTOR s{ XMLoadFloat3(&scales[index]) };

    XMMATRIX world{ XMMatrixAffineTransformation(s, XMQuaternionIdentity(), r, t) };
    XMStoreFloat4x4(&to_world[index], world);

    // avoiding contamination of matrices with translation information, when used to transform vectors.
    // NOTE: see section 8.2.2 of "Introduction to 3D Game Programming with DirectX® 12" by Frank D. Luna
    world.r[3] = XMVectorSet(0.f, 0.f, 0.f, 1.f);

    XMMATRIX inverse_world{ XMMatrixInverse(nullptr, world) };
    XMStoreFloat4x4(&inv_world[index], inverse_world);

    has_transforms[index] = 1;
}

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
        has_transforms[entity_index] = 0;
    }
    else {
        assert(positions.size() == entity_index);
        rotations.emplace_back(info.rotation);
        orientations.emplace_back(calculate_orientation(math::vec4{ info.rotation }));
        positions.emplace_back(info.position);
        scales.emplace_back(info.scale);
        to_world.emplace_back();
        inv_world.emplace_back();
        has_transforms.emplace_back((u8)0);
    }

    // NOTE: each entity has a transform component. Therefore ids for transform components are exactly the same as entity ids.
    return component(transform_id{ entity.get_id() });
}


void remove([[maybe_unused]] component comp) {
    // NOTE: technically we don't have to clean up positions, rotations and scales utl::vectors there,
    // since create() manages allocation quite well.
    assert(comp.is_valid());
}

void
get_transform_matrices(const game_entity::entity_id id, math::mat4x4& world, math::mat4x4& inverse_world) {
    assert(game_entity::entity{ id }.is_valid());

    const id::id_type entity_index{ id::index(id) };
    if (!has_transforms[entity_index]) {
        calculate_transform_matrices(entity_index);
    }

    world = to_world[entity_index];
    inverse_world = inv_world[entity_index];
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