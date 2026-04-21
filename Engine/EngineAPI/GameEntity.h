#pragma once

#include "../Components/ComponentsCommon.h"
#include "TransformComponent.h"
#include "ScriptComponent.h"

namespace mage {
	namespace game_entity {

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
			script::Component script() const;

		private:
			entity_id _id;
		};
	} // namespace game_entity


	namespace script {

		// base class, we don't want to instantiate "just" it
		class ScriptEntity : public game_entity::Entity {
		public:
			virtual ~ScriptEntity() = default;
			virtual void begin_play() {}
			virtual void update(float) {}

		protected:
			constexpr explicit ScriptEntity(game_entity::Entity entity)
				: game_entity::Entity{ entity.get_id() } {}
		};

		namespace detail {
			using script_ptr = std::unique_ptr<ScriptEntity>;
			using script_creator_fn_ptr = script_ptr(*)(game_entity::Entity entity);
			using string_hash = std::hash<std::string>;

			u8 register_script(size_t, script_creator_fn_ptr);

			template<class ScriptClass>
			script_ptr create_script(game_entity::Entity entity) {
				assert(entity.is_valid());
				return std::make_unique<ScriptClass>(entity);
			}

#define REGISTER_SCRIPT(TYPE)																\
		class TYPE;																			\
		namespace {																			\
			const u8 _reg_##TYPE															\
			{ mage::script::detail::register_script(										\
				mage::script::detail::string_hash()(#TYPE),									\
				&mage::script::detail::create_script<TYPE>)};								\
		}
		// end of REGISTER_SCRIPT macro

		} // namespace detail

	} // namespace script
}