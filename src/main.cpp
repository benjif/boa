#include "boa/macros.h"
#include "boa/iteration.h"
#include "boa/gfx/asset/model.h"
#include "boa/gfx/renderer.h"
#include "boa/ecs/ecs.h"

int main(int argc, char **argv) {
    LOG_INFO("(Global) Started");

    boa::ecs::ComponentStore component_store;
    boa::ecs::EntityGroup entity_group;

    auto sponza = entity_group.new_entity();
    entity_group.enable<boa::ecs::Model>(sponza);
    entity_group.enable<boa::ecs::Transform>(sponza);

    //auto domino_crown2 = entity_group.new_entity();
    //entity_group.enable<boa::ecs::Model>(domino_crown2);

    {
        auto &transform_component = entity_group.get_component<boa::ecs::Transform>(sponza);
        transform_component.orientation = glm::quat(0.0f, 0.0f, 0.0f, 0.0f);
        transform_component.translation = glm::vec3(2.0f, 0.0f, 2.0f);
        transform_component.scale = glm::vec3(0.01f, 0.01f, 0.01f);
        transform_component.update();
    }

    boa::gfx::Model sponza_model;
    sponza_model.open_gltf_file("models/sponza/glTF/Sponza.gltf");

    boa::gfx::Renderer renderer;
    uint32_t sponza_model_id = renderer.load_model(sponza_model, "sponza");

    {
        auto &model_component = entity_group.get_component<boa::ecs::Model>(sponza);
        model_component.id = sponza_model_id;
    }

    /*{
        auto &model_component = entity_group.get_component<boa::ecs::Model>(domino_crown2);
        model_component.id = domino_crown_model_id;
    }*/

    //boa::gfx::Skybox daytime = renderer.load_skybox("skybox/daytime.png");
    //renderer.set_skybox(daytime);

    renderer.run();

    LOG_INFO("(Global) Exiting");
    return EXIT_SUCCESS;
}