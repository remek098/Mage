#include "Transform.h"
#include "Entity.h"

namespace mage::transform {

    namespace {
        utl::vector<math::vec3> positions;
        utl::vector<math::vec4> rotations;
        utl::vector<math::vec3> scales;
    } // anonymous namespace

    Component create_transform(const InitInfo& info, game_entity::Entity entity) {
        assert(entity.is_valid());
        const id::id_type entity_index{ id::index(entity.get_id()) };


        if ( positions.size() > entity_index ) {
            // when entity_index is pointing inside our array, we fill one of holes in array
            rotations[entity_index] = math::vec4(info.rotation);
            positions[entity_index] = math::vec3(info.position);
            scales[entity_index]    = math::vec3(info.scale);
        }
        else {
            assert(positions.size() == entity_index);
            rotations.emplace_back(info.rotation);
            positions.emplace_back(info.position);
            scales.emplace_back(info.scale);
        }

        return Component(transform_id{ (id::id_type)positions.size() - 1 });
    }


    void remove_transform(Component comp) {
        assert(comp.is_valid());
    }


    math::vec4 Component::rotation() const {
        assert(is_valid());
        return rotations[id::index(_id)];
    }
    
    math::vec3 Component::position() const {
        assert(is_valid());
        return positions[id::index(_id)];
    }
    
    math::vec3 Component::scale() const {
        assert(is_valid());
        return scales[id::index(_id)];
    }
}