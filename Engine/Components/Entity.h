#pragma once

#include "ComponentsCommon.h"

namespace mage {

#define INIT_INFO(component) namespace component { struct init_info; }

	// forward decleration
	INIT_INFO(transform)


#undef INIT_INFO


	namespace game_entity {
		struct entity_info {
			transform::init_info* tranform{ nullptr };
		};

		entity create_game_entity(const entity_info& info);
		void remove_game_entity(entity e);

		bool is_alive(entity e);
	}
}