#pragma once

#include "ComponentsCommon.h"

namespace mage::transform {

struct init_info {
	f32 position[3]{};
	f32 rotation[4]{};
	f32 scale[3]{1.f, 1.f, 1.f};
};


struct component_flags {
	enum flasg : u32 {
		rotation = 0x01,
		orientation = 0x02,
		position = 0x04,
		scale = 0x08,

		all = rotation | orientation | position | scale
	};
};

struct component_cache {
	math::vec4		rotation;
	math::vec3		orientation;
	math::vec3		position;
	math::vec3		scale;
	transform_id	id;
	u32				flags;
};

// NOTE: every entity has transform component and entity keeps track of generations of itself,
// managing transform components with usage of these 2 functions below
component		create(const init_info& info, game_entity::entity entity);
void			remove(component comp);

void get_transform_matrices(const game_entity::entity_id id, math::mat4x4& world, math::mat4x4& inverse_world);

/// <summary>
/// Takes in array of ids and returns array of flags that lets us know if entity has been moved, rotated or scaled during the
/// previous frame.
/// </summary>
void get_updated_components_flags(const game_entity::entity_id* const ids, u32 count, u8* const flags);
void update(const component_cache* const cache, u32 count);
}