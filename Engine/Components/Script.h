#pragma once

#include "ComponentsCommon.h"

namespace mage::script {

	struct InitInfo {
		// expects game_entity::Entity as param
		detail::script_creator_fn_ptr script_creator;
	};

	// NOTE: every entity has transform component and entity keeps track of generations of itself,
	// managing transform components with usage of these 2 functions below
	Component		create(const InitInfo& info, game_entity::Entity entity);
	void			remove(Component c);
}