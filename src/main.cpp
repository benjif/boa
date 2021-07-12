#include "boa/macros.h"
#include "boa/iteration.h"
#include "boa/gfx/asset/model.h"
#include "boa/gfx/renderer.h"
#include "boa/ecs/component.h"
#include "boa/ecs/entity.h"

int main(int argc, char **argv) {
    LOG_INFO("(Global) Started");

    boa::ecs::ComponentStore component_store;
    boa::ecs::EntityGroup entity_group;

    auto domino_crown = entity_group.new_entity();
    entity_group.enable<boa::ecs::Model>(domino_crown);
    entity_group.enable<boa::ecs::Transform>(domino_crown);

    auto &transform_component = entity_group.get_component<boa::ecs::Transform>(domino_crown);
    transform_component.orientation = glm::quat(0.0f, 0.0f, 0.0f, 0.0f);
    transform_component.translation = glm::vec3(0.0f);
    transform_component.scale = glm::vec3(2.0f, 2.0f, 2.0f);
    transform_component.update();

    boa::gfx::Model domino_crown_model;
    domino_crown_model.open_gltf_file("models/domino_crown.gltf");

    boa::gfx::Renderer renderer;
    uint32_t domino_crown_model_id = renderer.load_model(domino_crown_model, "domino_crown");

    auto &component = entity_group.get_component<boa::ecs::Model>(domino_crown);
    component.id = domino_crown_model_id;

    //boa::gfx::Skybox daytime = renderer.load_skybox("skybox/daytime.png");
    //renderer.set_skybox(daytime);

    renderer.run();

    LOG_INFO("(Global) Exiting");
    return EXIT_SUCCESS;
}