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
        
        
        using script_registry = std::unordered_map<size_t, detail::script_creator_fn_ptr>;
        
        script_registry& registery() {
            // NOTE: putting this static variable in function to avoid shenanigans with 
            // initialization order of static data.
            static script_registry reg;
            return reg;
        }

        

        /*
        NOTE: 
        game code dll only holds game code and functions for instantiation
        EngineDll.dll holds the data and engine code 
        (e.g. pointers to game code's functions, which engine would later invoke when needed)

        Data is exchanged via Level Editor
        */
#ifdef USE_WITH_EDITOR
        utl::vector<std::string>& script_names() {
            // NOTE: putting this static variable in function to avoid shenanigans with 
            // initialization order of static data.
            static utl::vector<std::string> names;
            return names;
        }
#endif


        // if generations match (as they should), and we have a valid entity (script_entity is still entity)
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
            bool result = registery().insert(script_registry::value_type{ tag, func }).second;
            assert(result);
            return result;
        }

        script_creator_fn_ptr get_script_creator(size_t tag) {
            // script_registry is defined as std::unordered_map
            // remember we put {tag, func_key} pair inside registry when we register a script.
            auto script = mage::script::registery().find(tag);
            assert(script != mage::script::registery().end() && script->first == tag);
            return script->second; // ptr to creation function
        }



#ifdef USE_WITH_EDITOR
        u8 add_script_name(const char* name) {
            script_names().emplace_back(name);
            return true;
        }
#endif // USE_WITH_EDITOR

    } // namespace detail

    component create(const init_info& info, game_entity::entity entity) {
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
        // get position of where new script_entity was added, ofc it is end of script_entities array
        const id::id_type index{ (id::id_type)script_entities.size() };
        script_entities.emplace_back(info.script_creator(entity));
        assert(script_entities.back()->get_id() == entity.get_id());    // should always be true
        id_mapping[id::index(id)] = index;
        return component{ id };
    }

    // swap the element at the end of script_entities array with the element we want to delete.
    // reset id_mapping for deleted component,
    // set new index that originally pointed to last element to the new position.
    void remove(component c) {
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

#ifdef USE_WITH_EDITOR
#include <atlsafe.h>

extern "C" __declspec(dllexport)
LPSAFEARRAY get_script_names() {
    const u32 size = (u32)mage::script::script_names().size();
    if ( !size ) return nullptr;

    // NOTE: all this hustle just to get it to .NET/C# framework

    // https://learn.microsoft.com/en-us/cpp/atl/reference/ccomsafearray-class?view=msvc-170
    CComSafeArray<BSTR> names(size);
    for ( u32 i = 0; i < size; ++i ) {
        // https://learn.microsoft.com/en-us/cpp/atl/reference/ccomsafearray-class?view=msvc-170#setat
        // A2BSTR_EX -> macro used to convert LPCSTR (ANSI string) to BSTR (Basic string / COM string)
        names.SetAt(i, A2BSTR_EX(mage::script::script_names()[i].c_str()), false);
    }

    // returns pointer to SAFEARRAY object
    // also the task of freeing this memory is "moved" to C# side of things -> so editor (garbage collected)
    return names.Detach();
}
#endif // USE_WITH_EDITOR