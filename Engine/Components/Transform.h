#pragma once

#include "ComponentsCommon.h"

namespace mage::transform {

	struct init_info {
		f32 position[3]{};
		f32 rotation[4]{};
		f32 scale[3]{1.f, 1.f, 1.f};
	};

	// NOTE: every entity has transform component and entity keeps track of generations of itself,
	// managing transform components with usage of these 2 functions below
	component		create_transform(const init_info& info, game_entity::entity entity);
	void			remove_transform(component comp);
}