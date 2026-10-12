#pragma once

#include "../Components/ComponentsCommon.h"
#include "TransformComponent.h"
#include "ScriptComponent.h"

namespace mage {
	namespace game_entity {

		// in lower case because entity_id undercover is just an 
		// id::id_type which is u32 for now. Can be diffrent unsigned integer type
		DEFINE_TYPED_ID(entity_id);

		class entity {
		public:
			constexpr explicit entity(entity_id id) : _id{ id } {}
			constexpr entity() : _id{ id::invalid_id } {}

			[[nodiscard]] constexpr entity_id get_id() const { return _id; }
			[[nodiscard]] constexpr bool is_valid() const { return id::is_valid(_id); }

			[[nodiscard]] transform::component transform() const;
			[[nodiscard]] script::component script() const;

			[[nodiscard]] math::vec4 rotation() const { return transform().rotation(); }
			[[nodiscard]] math::vec3 orientation() const { return transform().orientation(); }
			[[nodiscard]] math::vec3 position() const { return transform().position(); }
			[[nodiscard]] math::vec3 scale() const { return transform().scale(); }

		private:
			entity_id _id;
		};
	} // namespace game_entity


	namespace script {

		// base class, we don't want to instantiate "just" it
		class script_entity : public game_entity::entity {
		public:
			virtual ~script_entity() = default;
			virtual void begin_play() {}
			virtual void update(float) {}

		protected:
			constexpr explicit script_entity(game_entity::entity entity)
				: game_entity::entity{ entity.get_id() } {}

			void set_rotation(math::vec4 rotation_quaternion) { set_rotation(this, rotation_quaternion); }
			void set_orientation(math::vec3 orientation_vector) { set_orientation(this, orientation_vector); }
			void set_position(math::vec3 position) { set_position(this, position); }
			void set_scale(math::vec3 scale) { set_scale(this, scale); }

			static void set_rotation(const game_entity::entity* const entity, math::vec4 rotation_quaternion);
			static void set_orientation(const game_entity::entity* const entity, math::vec3 orientation_vector);
			static void set_position(const game_entity::entity* const entity, math::vec3 position);
			static void set_scale(const game_entity::entity* const entity, math::vec3 scale);
		};

		namespace detail {
			using script_ptr = std::unique_ptr<script_entity>;
			using script_creator_fn_ptr = script_ptr(*)(game_entity::entity entity);
			using string_hash = std::hash<std::string>;


			u8 register_script(size_t, script_creator_fn_ptr);

			// NOTE: exporting this for the same reason as why we want to export get_script_names() (in Script.cpp file)
			// to ask game_code_dll what the script_creator_fn_ptrs are
#ifdef USE_WITH_EDITOR
			extern "C" __declspec(dllexport)
#endif
			script_creator_fn_ptr get_script_creator(size_t tag);


			template<class ScriptClass>
			script_ptr create_script(game_entity::entity entity) {
				assert(entity.is_valid());
				return std::make_unique<ScriptClass>(entity);
			}

#ifdef USE_WITH_EDITOR
			u8 add_script_name(const char* name);
#define REGISTER_SCRIPT(TYPE)																\
		namespace {																			\
			const u8 _reg_##TYPE															\
			{ mage::script::detail::register_script(										\
				mage::script::detail::string_hash()(#TYPE),								\
				&mage::script::detail::create_script<TYPE>)};								\
			const u8 _name_##TYPE															\
			{ mage::script::detail::add_script_name(#TYPE) };								\
		}
		// end of REGISTER_SCRIPT macro
#else
#define REGISTER_SCRIPT(TYPE)																\
		namespace {																			\
			const u8 _reg_##TYPE															\
			{ mage::script::detail::register_script(										\
				mage::script::detail::string_hash()(#TYPE),									\
				&mage::script::detail::create_script<TYPE>)};								\
		}
		// end of REGISTER_SCRIPT macro

#endif // USE_WITH_EDITOR
		} // namespace detail

	} // namespace script
}