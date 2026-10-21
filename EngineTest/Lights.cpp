#include "EngineAPI/GameEntity.h"
#include "EngineAPI/Light.h"
#include "EngineAPI/TransformComponent.h"
#include "Graphics/Renderer.h"

using namespace mage;

namespace {
const u64 left_set{ 0 };
const u64 right_set{ 1 };

utl::vector<gfx::light> lights;


constexpr math::vec3
rgb_to_color(u8 r, u8 g, u8 b) { return { r / 255.f, g / 255.f, b / 255.f }; }
} // anonymous namespace

game_entity::entity create_one_game_entity(math::vec3 position, math::vec3 rotation, const char *script_name);
void remove_game_entity(game_entity::entity_id id);

void
generate_lights() {
    // LEFT SET
    gfx::light_init_info info{};
    info.light_set_key = left_set;
    info.entity_id = create_one_game_entity({}, { 0,0,0 }, nullptr).get_id();
    info.type = gfx::light::directional;
    info.color = rgb_to_color(174, 174, 174);
    info.intensity = 1.f;
    info.directional_params = {};
    // info.is_enabled = true;
    lights.emplace_back(gfx::create_light(info));
    
    // shines from above
    info.entity_id = create_one_game_entity({}, { math::pi * 0.5f, 0, 0 }, nullptr).get_id();
    info.color = rgb_to_color(17, 27, 48);
    lights.emplace_back(gfx::create_light(info));

    // shines from below
    info.entity_id = create_one_game_entity({}, { -math::pi * 0.5f, 0, 0 }, nullptr).get_id();
    info.color = rgb_to_color(63, 47, 30);
    lights.emplace_back(gfx::create_light(info));
    // light 2 and 3 are to simulate Skylight and light that's reflected from the ground. -> similar to WPF geometry viewer in MageEditor


    // RIGHT SET
    info.light_set_key = right_set;
    info.entity_id = create_one_game_entity({}, { 0, 0, 0 }, nullptr).get_id();
    info.color = rgb_to_color(150, 100, 200);
    lights.emplace_back(gfx::create_light(info));

    // shines from above
    info.entity_id = create_one_game_entity({}, { math::pi * 0.5f, 0, 0 }, nullptr).get_id();
    info.color = rgb_to_color(17, 27, 48);
    lights.emplace_back(gfx::create_light(info));

    // shines from below
    info.entity_id = create_one_game_entity({}, { -math::pi * 0.5f, 0, 0 }, nullptr).get_id();
    info.color = rgb_to_color(63, 47, 30);
    lights.emplace_back(gfx::create_light(info));
    // light 2 and 3 are to simulate Skylight and light that's reflected from the ground. -> similar to WPF geometry viewer in MageEditor
}

void remove_lights() {
    for (auto &light : lights) {
        const game_entity::entity_id id{ light.entity_id() };
        gfx::remove_light(light.get_id(), light.light_set_key());
        remove_game_entity(id);
    }
    lights.clear();
}
