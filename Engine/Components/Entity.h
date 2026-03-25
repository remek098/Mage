#pragma once

#include "ComponentsCommon.h"

namespace mage {

#define INIT_INFO(component) namespace component { struct InitInfo; }

	// forward decleration
	INIT_INFO(transform)


#undef INIT_INFO


	namespace game_entity {
		struct EntityInfo {
			transform::InitInfo* tranform{ nullptr };
		};

		Entity create_game_entity(const EntityInfo& info);
		void remove_game_entity(Entity e);

		bool is_alive(Entity e);
	}
}