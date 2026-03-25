#pragma once

#include "../Components/ComponentsCommon.h"
#include "TransformComponent.h"

namespace mage::game_entity {

	// in lower case because entity_id undercover is just an 
	// id::id_type which is u32 for now. Can be diffrent unsigned integer type
	DEFINE_TYPED_ID(entity_id);

	class Entity {
	public:
		constexpr explicit Entity(entity_id id) : _id{ id } {}
		constexpr Entity() : _id{ id::invalid_id } {}

		constexpr entity_id get_id() { return _id; }
		constexpr bool is_valid() const { return id::is_valid(_id); }

		transform::Component transform() const;

	private:
		entity_id _id;
	};
}