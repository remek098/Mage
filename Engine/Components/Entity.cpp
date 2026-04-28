#include "Entity.h"
#include "Transform.h"
#include "Script.h"

namespace mage::game_entity {

	// anonymous namespace
	namespace {
		utl::vector<transform::Component>	transforms;
		utl::vector<script::Component>		scripts;

		utl::vector<id::gen_type>	generations;
		utl::deque<entity_id>		free_ids;
	} // end anyonymous namespace


	Entity create(const EntityInfo& info) {
		assert(info.tranform); // all game entities must have a transform component
		if ( !info.tranform ) return Entity{};

		entity_id id;

		// either reuse free existing id or add a new slot for it at the end of the array
		if ( free_ids.size() > id::min_deleted_elements ) {
			// pick the first one, check if it's one of those 'dead' entities
			id = free_ids.front();
			assert(!is_alive(id));
			// remove it from free_ids, increase it's generation
			free_ids.pop_front();
			id = entity_id{ id::new_generation(id) };
			++generations[id::index(id)];
		}
		else {
			// if we dind't run out of space for our entities yet, just add new entity to our vector and set it's 
			// generation to 0
			id = entity_id{ (id::id_type)generations.size() };
			generations.push_back(0);
			
			// adding default transform::component at the end each time we create new "default" entity
			// NOTE: don't call resize(), so that the number of memory allocations stays as low as possible
			transforms.emplace_back();
			scripts.emplace_back();
		}

		const Entity new_entity{ id };
		const id::id_type index{ id::index(id) };

		// create transform component 
		assert(!transforms[index].is_valid());
		transforms[index] = transform::create(*info.tranform, new_entity);
		if ( !transforms[index].is_valid() ) return {};

		// create script component
		if ( info.script && info.script->script_creator ) {
			assert(!scripts[index].is_valid());
			scripts[index] = script::create(*info.script, new_entity);
			assert(scripts[index].is_valid());
		}

		return new_entity;
	}
	
	void remove(entity_id id) {
		const id::id_type index{ id::index(id) };
		assert(is_alive(id));

		if ( scripts[index].is_valid() ) {
			script::remove(scripts[index]);
			scripts[index] = {}; // empty component with INVALID_ID
		}

		transform::remove(transforms[index]);
		transforms[index] = {};


		// mark as the id that can be reused later
		free_ids.push_back(id);
	}

	bool is_alive(entity_id id) {
		assert(id::is_valid(id));
		const id::id_type index{ id::index(id) };

		assert(index < generations.size());
		assert(generations[index] == id::generation(id));
		return (generations[index] == id::generation(id) && transforms[index].is_valid());
	}


	transform::Component Entity::transform() const {
		assert(is_alive(_id));
		const id::id_type index{ id::index(_id) };
		return transforms[index];
	}

	script::Component Entity::script() const {
		assert(is_alive(_id));
		const id::id_type index{ id::index(_id) };
		return scripts[index];
	}
}