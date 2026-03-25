#include "Entity.h"
#include "Transform.h"

namespace mage::game_entity {

	// anonymous namespace
	namespace {
		utl::vector<transform::Component>	transforms;

		utl::vector<id::gen_type>	generations;
		utl::deque<entity_id>		free_ids;
	} // end anyonymous namespace


	Entity create_game_entity(const EntityInfo& info) {
		assert(info.tranform); // all game entities must have a transform component
		if ( !info.tranform ) return Entity{};

		entity_id id;

		// either reuse free existing id or add a new slot for it at the end of the array
		if ( free_ids.size() > id::min_deleted_elements ) {
			// pick the first one, check if it's one of those 'dead' entities
			id = free_ids.front();
			assert(!is_alive(Entity{ id }));
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
		}

		const Entity new_entity{ id };
		const id::id_type index{ id::index(id) };

		// create transform component 
		assert(!transforms[index].is_valid());
		transforms[index] = transform::create_transform(*info.tranform, new_entity);
		if ( !transforms[index].is_valid() ) return {};

		return new_entity;
	}
	
	void remove_game_entity(Entity e) {
		const entity_id id{ e.get_id() };
		const id::id_type index{ id::index(id) };
		assert(is_alive(e));

		if ( is_alive(e) ) {
			transform::remove_transform(transforms[index]);
			transforms[index] = {};

			// mark as the id that can be reused later
			free_ids.push_back(id);
		}
	}

	bool is_alive(Entity e) {
		assert(e.is_valid());
		const entity_id id{ e.get_id() };
		const id::id_type index{ id::index(id) };


		assert(index < generations.size());
		assert(generations[index] == id::generation(id));
		return (generations[index] == id::generation(id) && transforms[index].is_valid());
	}


	transform::Component Entity::transform() const {
		assert(is_alive(*this));
		const id::id_type index{ id::index(_id) };
		return transforms[index];
	}
}