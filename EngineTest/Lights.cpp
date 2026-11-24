#include "EngineAPI/GameEntity.h"
#include "EngineAPI/Light.h"
#include "EngineAPI/TransformComponent.h"
#include "Graphics/Renderer.h"

#define RANDOM_LIGHTS 1

using namespace mage;


game_entity::entity create_one_game_entity(math::vec3 position, math::vec3 rotation, const char *script_name);
void remove_game_entity(game_entity::entity_id id);

namespace {
const u64 left_set{ 0 };
const u64 right_set{ 1 };
constexpr f32 inv_rand_max{ 1.f / RAND_MAX };

utl::vector<gfx::light> lights;


constexpr math::vec3
rgb_to_color(u8 r, u8 g, u8 b) { return { r / 255.f, g / 255.f, b / 255.f }; }

f32 random(f32 min = 0.f) { return std::max(min, rand() * inv_rand_max); }

void 
create_light(math::vec3 position, math::vec3 rotation, gfx::light::type type, u64 light_set_key) {
    game_entity::entity_id entity_id{ create_one_game_entity(position, rotation, nullptr).get_id() };
    gfx::light_init_info info{};
    info.entity_id = entity_id;
    info.type = type;
    info.light_set_key = light_set_key;
    info.intensity = 1.f;

    info.color = { random(0.2f), random(0.2f) , random(0.2f) };

#if RANDOM_LIGHTS
    if (type == gfx::light::point) {
        info.point_params.range = random(0.5f) * 2.f;
        info.point_params.attenuation = { 1, 1, 1 };
    }
    else if (type == gfx::light::spot) {
        info.spot_params.range = random(0.5f) * 2.f;
        info.spot_params.umbra = (random(0.5f) - 0.4f) * math::pi;
        info.spot_params.penumbra = info.spot_params.umbra + (0.1f * math::pi);
        info.spot_params.attenuation = { 1, 1,1 };
    }
#else
    if (type == gfx::light::point) {
        info.point_params.range = 1.f;
        info.point_params.attenuation = { 1, 1, 1 };
    }
    else if (type == gfx::light::spot) {
        info.spot_params.range = 2.f;
        info.spot_params.umbra = 0.1f * math::pi;
        info.spot_params.penumbra = info.spot_params.umbra + (0.1f * math::pi);
        info.spot_params.attenuation = { 1, 1,1 };
    }
#endif

    gfx::light light{ gfx::create_light(info) };
    assert(light.is_valid());
    lights.push_back(light);
}
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
    info.color = rgb_to_color(150, 100, 200); // medium bright, saturated purple
    lights.emplace_back(gfx::create_light(info));

    // shines from above
    info.entity_id = create_one_game_entity({}, { math::pi * 0.5f, 0, 0 }, nullptr).get_id();
    info.color = rgb_to_color(17, 27, 48);
    lights.emplace_back(gfx::create_light(info));

    // shines from below
    info.entity_id = create_one_game_entity({}, { -math::pi * 0.5f, 0, 0 }, nullptr).get_id();
    info.color = rgb_to_color(163, 47, 30); // pink from below
    lights.emplace_back(gfx::create_light(info));
    // light 2 and 3 are to simulate Skylight and light that's reflected from the ground. -> similar to WPF geometry viewer in MageEditor

#if !RANDOM_LIGHTS
    create_light({ 0, -3, 0 }, {}, gfx::light::point, left_set);
    create_light({ 0, 0, 1 }, {}, gfx::light::point, left_set);
    create_light({ 0, 3, 2.5f }, {}, gfx::light::point, left_set);
    create_light({ 0, 0, 7 }, {0, 3.14f, 0}, gfx::light::spot, left_set);
#else
    srand(37);

    constexpr math::vec3 scale{ 1.f, 0.5f, 1.f };
    constexpr i32 dim{ 5 }; // 1000 lights per set
    for(i32 x{-dim}; x < dim; ++x)
        for (i32 y{ 0 }; y < 2 * dim; ++y)
            for (i32 z{ -dim }; z < dim; ++z) {
                create_light({ (f32)(x * scale.x), (f32)(y * scale.y) , (f32)(z * scale.z) },
                             { 3.14f, random(), 0.f }, 
                             random() > 0.5f ? gfx::light::spot : gfx::light::point, left_set);
                create_light({ (f32)(x * scale.x), (f32)(y * scale.y) , (f32)(z * scale.z) },
                             { 3.14f, random(), 0.f },
                             random() > 0.5f ? gfx::light::spot : gfx::light::point, right_set);
            }
#endif
}

void remove_lights() {
    for (auto &light : lights) {
        const game_entity::entity_id id{ light.entity_id() };
        gfx::remove_light(light.get_id(), light.light_set_key());
        remove_game_entity(id);
    }
    lights.clear();
}
