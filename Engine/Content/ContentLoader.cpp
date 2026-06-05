#include "ContentLoader.h"
#include "..\Components\Entity.h"
#include "..\Components\Transform.h"
#include "..\Components\Script.h"

#if !defined(SHIPPING)

#include <fstream>
#include <filesystem>
#include <Windows.h>

namespace mage::content {
    
    namespace {
        enum component_type {
            transform,
            script,

            count
        };

        // helper variable reused while loading entities from binary save
        utl::vector<game_entity::entity> loaded_entities;
        transform::init_info transform_info{};
        script::init_info script_info{};

        bool read_transform(const u8*& data, game_entity::entity_info& info) {
            // checkout MageEditor>Components>Transform.cs>WriteToBinary() for refernce
            // basically we need to read pos, rot, scale -> all these are float[3]
            using namespace DirectX;
            f32 rotation[3];
            
            assert(!info.tranform);
            memcpy(&transform_info.position[0], data, sizeof(transform_info.position)); data += sizeof(transform_info.position);
            memcpy(&rotation[0], data, sizeof(rotation)); data += sizeof(rotation);
            memcpy(&transform_info.scale[0], data, sizeof(transform_info.scale)); data += sizeof(transform_info.scale);

            // convert from Euler to Quaternions
            XMFLOAT3A rot{ &rotation[0] };
            XMVECTOR quat{ XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3A(&rot)) };
            XMFLOAT4A rot_quat{};
            XMStoreFloat4A(&rot_quat, quat);
            memcpy(&transform_info.rotation[0], &rot_quat.x, sizeof(transform_info.rotation));

            info.tranform = &transform_info;
            return true;
        }

        bool read_script(const u8*& data, game_entity::entity_info& info) {
            assert(!info.script);
            // checkout MageEditor>Components>Script.cs>WriteToBinary() for refernce
            const u32 name_length = *data; data += sizeof(u32);
            if ( !name_length ) return false;

            // NOTE: if script's name is over 255 characters long, then something is terribly wrong,
            // either with the binary writer or with a game programmer \_:)_/
            assert(name_length < 256);
            char script_name[256];
            memcpy(&script_name[0], data, name_length); data += name_length;
            script_name[name_length] = 0; // make script_name a zero-terminated c-string.
            script_info.script_creator = script::detail::get_script_creator(script::detail::string_hash()(script_name));
            
            info.script = &script_info;
            return script_info.script_creator != nullptr;
        }

        // u8*& used as reference to pointer; 
        // first param will take pointer to a buffer -> it can be changed
        // 2nd param is filled in by function
        using component_reader = bool(*)(const u8*&, game_entity::entity_info&);
        component_reader component_readers[]{
            read_transform,
            read_script,
        };
        static_assert(_countof(component_readers) == component_type::count);
    } // anonymous namespace

    bool load_game() {
        // set the working directory to the .exe path
        wchar_t path[MAX_PATH];
        const u32 length = GetModuleFileName(0, &path[0], MAX_PATH);
        if ( !length || GetLastError() == ERROR_INSUFFICIENT_BUFFER ) return false;
        std::filesystem::path p{ path };
        SetCurrentDirectory(p.parent_path().wstring().c_str());

        // read game.bin and create entities.
        std::ifstream game("game.bin", std::ios::in | std::ios::binary);
        // reads buffer character after character and adds it into a buffer
        utl::vector<u8> buffer(std::istreambuf_iterator<char>(game), {});
        assert(buffer.size()); // non-zero size
        const u8* at = buffer.data();
        // gonna add it to a pointer whenever we read smth because we read 4 bytes (integer or float)
        constexpr u32 size_u32 = sizeof(u32);
        
        // MageEditor>GameProject>Project>SaveToBinary() for reference
        // on what is written in what order
        const u32 num_entities = *at; at += size_u32;
        
        if ( !num_entities ) return false; // if 0, we can't load anything
        
        for ( u32 entity_index = 0; entity_index < num_entities; ++entity_index ) {
            game_entity::entity_info info{};
            
            const u32 entity_type = *at; at += size_u32; // NOTE: not used yet, keep it for now
            const u32 num_components = *at; at += size_u32;
            if ( !num_components ) return false;

            for ( u32 component_index = 0; component_index < num_components; ++component_index ) {
                const u32 component_type = *at; at += size_u32;
                assert(component_type < component_type::count);
                if ( !component_readers[component_type](at, info) ) return false;
            }

            assert(info.tranform);
            game_entity::entity entity{ game_entity::create(info) };
            if ( !entity.is_valid() ) return false;
            loaded_entities.emplace_back(entity);
        } // end of for each entity found in save file

        assert(at == buffer.data() + buffer.size()); // it makes sure we read through whole game.bin file
        return true;
    }

    void unload_game() {
        for ( auto entity : loaded_entities ) {
            game_entity::remove(entity.get_id());
        }
    }
}

#endif // !defined(SHIPPING)