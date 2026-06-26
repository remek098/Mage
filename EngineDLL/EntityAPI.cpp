#include "Common.h"
// engine includes
#include "CommonHeaders.h"
#include "Id.h"
#include "../Components/Entity.h"
#include "../Components/Transform.h"
#include "../Components/Script.h"

using namespace mage;

namespace {
    struct transform_component {
        f32 position[3];
        f32 rotation[3];
        f32 scale[3];

        transform::init_info to_init_info() {
            using namespace DirectX;
            transform::init_info info{};
            memcpy(info.position, position, sizeof(position));
            memcpy(info.scale, scale, sizeof(scale));

            // kinda don't have to use aligned versions of XMFLOAT vectors
            XMFLOAT3A rot{ rotation };
            XMVECTOR quat{ XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3A(&rot)) };

            XMFLOAT4A rot_quat{};
            XMStoreFloat4A(&rot_quat, quat);
            memcpy(&info.rotation[0], &rot_quat.x, sizeof(info.rotation));

            return info;
        }
    };

    struct script_component {
        script::detail::script_creator_fn_ptr script_creator;
        
        script::init_info to_init_info() {
            script::init_info info{};
            info.script_creator = script_creator;
            return info;
        }
    };

    struct game_entity_desc {
        transform_component transform;
        script_component script;
    };


    game_entity::entity entity_from_id(id::id_type id) {
        return game_entity::entity{ game_entity::entity_id{id} };
    }
}


MAGE_ED_INTERFACE id::id_type CreateGameEntity(game_entity_desc* p_entity_desc) {
    assert(p_entity_desc);
    game_entity_desc& desc = *p_entity_desc;

    transform::init_info transform_info{ desc.transform.to_init_info() };
    script::init_info script_info{ desc.script.to_init_info() };

    // for now only 1 component in entity -> transform
    game_entity::entity_info entity_info{ 
        &transform_info,
        &script_info
    };

    return game_entity::create(entity_info).get_id();
}

MAGE_ED_INTERFACE void RemoveGameEntity(id::id_type id) {
    assert(id::is_valid(id));
    game_entity::remove(game_entity::entity_id{ id });
}