#pragma once

#include "ComponentsCommon.h"

namespace mage {

#define INIT_INFO(component) namespace component { struct init_info; }

	// forward decleration
	INIT_INFO(transform);
	INIT_INFO(script);


#undef INIT_INFO


	namespace game_entity {
		struct entity_info {
			transform::init_info*	tranform{ nullptr };
			script::init_info*		script{ nullptr };
		};

		entity create(const entity_info& info);
		void remove(entity_id id);

		bool is_alive(entity_id id);
	}
}