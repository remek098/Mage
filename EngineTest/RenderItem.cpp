#include <filesystem>
#include "CommonHeaders.h"
#include "Content/ContentToEngine.h"
#include "Graphics/Renderer.h"
#include "ShaderCompilation.h"
#include "Components/Entity.h"
#include "../ContentTools/Geometry.h"

using namespace mage;

game_entity::entity create_one_game_entity(math::vec3 position, math::vec3 rotation, const char *script_name);
void remove_game_entity(game_entity::entity_id id);

bool read_file(std::filesystem::path, std::unique_ptr<u8[]>&, u64&);

namespace {
id::id_type apartment_model_id{ id::invalid_id };
id::id_type human_model_id{ id::invalid_id };

id::id_type apartment_item_id{ id::invalid_id };
id::id_type human_item_id{ id::invalid_id };

game_entity::entity_id apartment_entity_id{ id::invalid_id };
game_entity::entity_id human_entity_id{ id::invalid_id };

id::id_type vs_id{ id::invalid_id };
id::id_type ps_id{ id::invalid_id };
id::id_type mat_id{ id::invalid_id };

std::unordered_map<id::id_type, game_entity::entity_id> render_item_entity_map;

[[nodiscard]] id::id_type
load_model(const char* path) {
    // load test model
    std::unique_ptr<u8[]> model;
    u64 size{ 0 };
    read_file(path, model, size);

    const id::id_type model_id{ content::create_resource(model.get(), content::asset_type::mesh) };
    assert(id::is_valid(model_id));
    return model_id;
}

void
load_shaders() {
    shader_file_info info{};
    info.file_name = "TestShader.hlsl";
    const char* shader_path{ "..\\..\\enginetest\\" };

    std::wstring defines[]{ L"ELEMENTS_TYPE=1", L"ELEMENTS_TYPE=3" };
    utl::vector<u32> keys;
    keys.emplace_back(tools::vertex_elements::elements_type::static_normal);
    keys.emplace_back(tools::vertex_elements::elements_type::static_normal_texture);

    utl::vector<std::wstring> extra_args{};

    info.function = "TestShaderVS";
    info.type = shader_type::vertex;
    utl::vector<std::unique_ptr<u8[]>> vertex_shaders;
    utl::vector<const u8*> vertex_shaders_pointers;
    for (u32 i{ 0 }; i < _countof(defines); ++i) {
        extra_args.clear();
        extra_args.emplace_back(L"-D");
        extra_args.emplace_back(defines[i]);
        vertex_shaders.emplace_back(std::move(compile_shader(info, shader_path, extra_args)));
        assert(vertex_shaders.back().get());
        vertex_shaders_pointers.emplace_back(vertex_shaders.back().get());
    }
    extra_args.clear();


    info.function = "TestShaderPS";
    info.type = shader_type::pixel;
    // no variations for pixel shader yet.
    auto ps = compile_shader(info, shader_path, extra_args);
    assert(ps.get());
    const u8* pixel_shaders_pointers[]{ ps.get() };

    vs_id = content::add_shader_group(vertex_shaders_pointers.data(), (u32)vertex_shaders_pointers.size(), keys.data());
    ps_id = content::add_shader_group(&pixel_shaders_pointers[0], 1, &u32_invalid_id);
}

void
create_material() {
    assert(id::is_valid(vs_id) && id::is_valid(ps_id)); // make sure shaders are compiled
    gfx::material_init_info info{};
    info.shader_ids[gfx::shader_type::vertex] = vs_id;
    info.shader_ids[gfx::shader_type::pixel] = ps_id;
    info.type = gfx::material_type::opaque;
    mat_id = content::create_resource(&info, content::asset_type::material);
}

void
remove_item(game_entity::entity_id entity_id, id::id_type item_id, id::id_type model_id) {
    if (id::is_valid(item_id)) {
        gfx::remove_render_item(item_id);
        auto pair = render_item_entity_map.find(item_id);
        if (pair != render_item_entity_map.end()) {
            remove_game_entity(pair->second);
        }

        if (id::is_valid(model_id)) {
            content::destroy_resource(model_id, content::asset_type::mesh);
        }
    }
}

} // anonymous_namespace


void
create_render_items() {
    // load a model, pretend it belongs to entity_id
    auto _1 = std::thread{ [] { apartment_model_id = load_model("..\\..\\x64\\apartment_model.model"); } };
    // auto _2 = std::thread{ [] { human_model_id = load_model("..\\..\\x64\\human_model.model"); } };
    auto _2 = std::thread{ [] { human_model_id = load_model("..\\..\\x64\\human_model_joined.model"); } };
    auto _3 = std::thread{ [] { load_shaders(); } };
    /* 
    load material:
    1) load textures (optional)
    2) load shaders for that material
    */

    apartment_entity_id = create_one_game_entity({}, {}, nullptr).get_id();
    human_entity_id = create_one_game_entity({1.22f, 0.f, 0.04f}, {}, nullptr /*"rotator_script"*/).get_id();

    _1.join();
    _2.join();
    _3.join();
    // add a render item using the model and it's materials.
    create_material();
    id::id_type human_materials[]{ mat_id , mat_id, mat_id, mat_id, mat_id/*, mat_id, mat_id, mat_id, mat_id, mat_id, mat_id*/ };
    // id::id_type materials[]{ mat_id, mat_id, mat_id, mat_id };
    id::id_type apartment_materials[]{ mat_id, mat_id, mat_id, mat_id, mat_id, mat_id, mat_id, mat_id,
                                       mat_id, mat_id, mat_id, mat_id, mat_id, mat_id, mat_id, mat_id,
                                       mat_id, mat_id, mat_id, mat_id, mat_id, mat_id, mat_id, mat_id,
                                       mat_id, mat_id, mat_id, mat_id, mat_id, mat_id, mat_id, mat_id,
                                       mat_id, mat_id, mat_id, mat_id, mat_id, mat_id, mat_id, mat_id,
                                       mat_id, mat_id, mat_id, mat_id, mat_id, mat_id, mat_id, mat_id,
                                       mat_id, mat_id, mat_id, mat_id, mat_id, mat_id, mat_id, mat_id, // 56
                                       mat_id, mat_id, mat_id, mat_id, mat_id, mat_id, mat_id, mat_id, // 64
                                       mat_id, mat_id, mat_id, mat_id, mat_id, mat_id, mat_id, mat_id, // 72
                                       mat_id, mat_id, mat_id, mat_id, mat_id, mat_id, mat_id, mat_id, // 80
                                       mat_id, mat_id 
    };

    apartment_item_id = gfx::add_render_item(apartment_entity_id, apartment_model_id, _countof(apartment_materials), &apartment_materials[0]);
    human_item_id = gfx::add_render_item(human_entity_id, human_model_id, _countof(human_materials), &human_materials[0]);
    // render_item_entity_map[item_id] = entity_id;
    render_item_entity_map[apartment_item_id] = apartment_entity_id;
    render_item_entity_map[human_item_id] = human_entity_id;

}

void 
destroy_render_items() {
    remove_item(apartment_entity_id, apartment_item_id, apartment_model_id);
    remove_item(human_entity_id, human_item_id, human_model_id);
    
    // remove material
    if (id::is_valid(mat_id)) content::destroy_resource(mat_id, content::asset_type::material);

    // remove shaders and textures
    if (id::is_valid(vs_id)) content::remove_shader_group(vs_id);
    if (id::is_valid(ps_id)) content::remove_shader_group(ps_id);
}

void
get_render_items(id::id_type* items, [[maybe_unused]] u32 count) {
    assert(count == 2);
    items[0] = apartment_item_id;
    items[1] = human_item_id;
}