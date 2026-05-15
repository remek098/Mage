#pragma once

#include "ComponentsCommon.h"

namespace mage::script {

	struct init_info {
		// expects game_entity::entity as param
		detail::script_creator_fn_ptr script_creator;
	};

	// NOTE: every entity has transform component and entity keeps track of generations of itself,
	// managing transform components with usage of these 2 functions below
	component		create(const init_info& info, game_entity::entity entity);
	void			remove(component c);
}