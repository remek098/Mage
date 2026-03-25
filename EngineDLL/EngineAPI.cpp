#ifndef MAGE_ED_INTERFACE
// extern "C" to avoid name mangling in cpp
#define MAGE_ED_INTERFACE extern "C" __declspec(dllexport)
#endif

// engine includes
#include "CommonHeaders.h"
#include "Id.h"
#include "../Components/Entity.h"
#include "../Components/Transform.h"

using namespace mage;

namespace {
    struct TransformComponent {
        f32 position[3];
        f32 rotation[3];
        f32 scale[3];

        transform::InitInfo to_init_info() {
            using namespace DirectX;
            transform::InitInfo info{};
            memcpy(info.position, position, sizeof(f32) * _countof(position));
            memcpy(info.scale, scale, sizeof(f32) * _countof(scale));

            // kinda don't have to use aligned versions of XMFLOAT vectors
            XMFLOAT3A rot{ rotation };
            XMVECTOR quat{ XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3A(&rot)) };

            XMFLOAT4A rot_quat{};
            XMStoreFloat4A(&rot_quat, quat);
            memcpy(&info.rotation[0], &rot_quat.x, sizeof(f32) * _countof(info.rotation));

            return info;
        }
    };

    struct GameEntityDesc {
        TransformComponent transform;
    };


    game_entity::Entity entity_from_id(id::id_type id) {
        return game_entity::Entity{ game_entity::entity_id{id} };
    }
}


MAGE_ED_INTERFACE id::id_type CreateGameEntity(GameEntityDesc* p_entity_desc) {
    assert(p_entity_desc);
    GameEntityDesc& desc = *p_entity_desc;

    transform::InitInfo transform_info{ desc.transform.to_init_info() };
    
    // for now only 1 component in entity -> transform
    game_entity::EntityInfo entity_info{ &transform_info };

    return game_entity::create_game_entity(entity_info).get_id();
}

MAGE_ED_INTERFACE void RemoveGameEntity(id::id_type id) {
    assert(id::is_valid(id));
    game_entity::remove_game_entity(entity_from_id(id));
}