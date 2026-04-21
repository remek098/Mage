#include "Script.h"
#include "Entity.h"

namespace mage::script {
    
    namespace {

        // double indexing id_mapping -> script_entities.
        // to make sure we have a tightly packed vector/array
        utl::vector<detail::script_ptr> script_entities;
        utl::vector<id::id_type>        id_mapping;

        utl::vector<id::gen_type>       generations;
        utl::vector<script_id>          free_ids;
        
        
        using script_registery = std::unordered_map<size_t, detail::script_creator_fn_ptr>;
        
        script_registery& registery() {
            // NOTE: putting this static variable in function to avoid shenanigans with 
            // initialization order of static data.
            static script_registery reg;
            return reg;
        }


        // if generations match (as they should), and we have a valid entity (ScriptEntity is still Entity)
        // return true
        bool exists(script_id id) {
            assert(id::is_valid(id));
            const id::id_type index{ id::index(id) };
            assert(index < generations.size() && id_mapping[index] < script_entities.size());
            assert(generations[index] == id::generation(id));

            
            return (generations[index] == id::generation(id)) &&
                    script_entities[id_mapping[index]] &&
                    script_entities[id_mapping[index]]->is_valid();

        }
    } // anonymous namespace

    namespace detail {
        u8 register_script(size_t tag, script_creator_fn_ptr func) {
            bool result = registery().insert(script_registery::value_type{ tag, func }).second;
            assert(result);
            return result;
        }
    } // namespace detail

    Component create(const InitInfo& info, game_entity::Entity entity) {
        assert(entity.is_valid());
        assert(info.script_creator);

        script_id id;
        if ( free_ids.size() > id::min_deleted_elements ) {
            id = free_ids.front();
            assert(!exists(id)); // assert that id doesn't exist
            // free, reuse by increasing generation for that id slot
            free_ids.pop_back();
            id = script_id{ id::new_generation(id) };
            ++generations[id::index(id)];
        }
        else {
            // that means we got to add new slots at the end of arrays
            id = script_id{ (id::id_type)id_mapping.size() };
            id_mapping.emplace_back();
            generations.push_back(0);
        }

        assert(id::is_valid(id));
        script_entities.emplace_back(info.script_creator(entity));
        assert(script_entities.back()->get_id() == entity.get_id());    // should always be true
        // get position of where new ScriptEntity was added, ofc it is end of script_entities array
        const id::id_type index{ (id::id_type)script_entities.size() };
        id_mapping[id::index(id)] = index;
        return Component{ id };
    }

    // swap the element at the end of script_entities array with the element we want to delete.
    // reset id_mapping for deleted Component,
    // set new index that originally pointed to last element to the new position.
    void remove(Component c) {
        assert(c.is_valid() && exists(c.get_id()));
        const script_id id{ c.get_id() };
        const id::id_type index{ id_mapping[id::index(id)] };
        // get id of last script.
        const script_id last_id{ script_entities.back()->script().get_id() };
        utl::erase_unordered(script_entities, index);
        id_mapping[id::index(last_id)] = index;
        id_mapping[id::index(id)] = id::invalid_id;
    }
}