#include "boa/macros.h"
#include "boa/iteration.h"
#include "boa/gfx/asset/gltf_model.h"
#include "boa/gfx/renderer.h"
#include "boa/ecs/ecs.h"
#include <chrono>

int main(int argc, char **argv) {
    LOG_INFO("(Global) Started");

    boa::ecs::ComponentStore component_store;
    boa::ecs::EntityGroup entity_group;

    auto sponza = entity_group.new_entity();
    entity_group.enable<boa::ecs::Renderable>(sponza);
    entity_group.enable<boa::ecs::Transformable>(sponza);

    auto &sponza_transform_component = entity_group.get_component<boa::ecs::Transformable>(sponza);
    {
        sponza_transform_component.orientation = glm::quat(0.0f, 0.0f, 0.0f, 0.0f);
        sponza_transform_component.translation = glm::vec3(2.0f, 0.0f, 2.0f);
        sponza_transform_component.scale = glm::vec3(0.02f, 0.02f, 0.02f);
        sponza_transform_component.update();
    }

    auto box_animated = entity_group.new_entity();
    entity_group.enable<boa::ecs::Renderable>(box_animated);
    entity_group.enable<boa::ecs::Transformable>(box_animated);

    auto &box_transform_component = entity_group.get_component<boa::ecs::Transformable>(box_animated);
    {
        box_transform_component.orientation = glm::quat(0.0f, 0.0f, 0.0f, 0.0f);
        box_transform_component.translation = glm::vec3(0.0f, 1.0f, 0.0f);
        box_transform_component.scale = glm::vec3(1.0f, 1.0f, 1.0f);
        box_transform_component.update();
    }

    boa::gfx::glTFModel sponza_model;
    sponza_model.open_gltf_file("models/sponza/glTF/Sponza.gltf");

    boa::gfx::glTFModel box_animated_model;
    box_animated_model.open_gltf_file("models/BoxAnimated.gltf");

    boa::gfx::Renderer renderer;

    uint32_t sponza_model_id = renderer.load_model(sponza_model, "sponza");
    {
        auto &model_component = entity_group.get_component<boa::ecs::Renderable>(sponza);
        model_component.id = sponza_model_id;
    }
    uint32_t box_animated_model_id = renderer.load_model(box_animated_model, "box_animated");
    {
        auto &model_component = entity_group.get_component<boa::ecs::Renderable>(box_animated);
        model_component.id = box_animated_model_id;
    }

    auto &window = renderer.get_window();
    auto &camera = renderer.get_camera();
    auto &keyboard = renderer.get_keyboard();
    auto &mouse = renderer.get_mouse();

    //auto start_time = std::chrono::high_resolution_clock::now();
    renderer.set_per_frame_callback([&](float time_change) {
        //auto current_time = std::chrono::high_resolution_clock::now();
        //float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

        time_change *= 60.0f;

        boa::gfx::Camera::DirectionFlags directions{};

        if (keyboard.key(boa::input::KEY_W))
            directions |= boa::gfx::Camera::DirectionFlags::Forward;
        if (keyboard.key(boa::input::KEY_A))
            directions |= boa::gfx::Camera::DirectionFlags::Left;
        if (keyboard.key(boa::input::KEY_S))
            directions |= boa::gfx::Camera::DirectionFlags::Backward;
        if (keyboard.key(boa::input::KEY_D))
            directions |= boa::gfx::Camera::DirectionFlags::Right;

        camera.update_position(time_change, directions);

        if (window.get_cursor_disabled()) {
            glm::dvec2 cursor_change = mouse.last_movement();
            camera.update_target(cursor_change);
        }
    });

    keyboard.set_callback([&](int key, int action, int mods) {
        if (action == boa::input::RELEASE) {
            if (key == boa::input::KEY_LEFT_SHIFT)
                camera.set_movement_speed(0.05f);
        } else if (action == boa::input::PRESS) {
            if (key == boa::input::KEY_ESCAPE) {
                if (window.get_cursor_disabled()) {
                    window.set_cursor_disabled(false);
                    renderer.set_ui_mouse_enabled(true);
                } else {
                    window.set_cursor_disabled(true);
                    renderer.set_ui_mouse_enabled(false);
                }
            } else if (key == boa::input::KEY_LEFT_SHIFT) {
                camera.set_movement_speed(0.20f);
            }
        }
    });

    //boa::gfx::Skybox daytime = renderer.load_skybox("skybox/daytime.png");
    //renderer.set_skybox(daytime);

    renderer.run();

    LOG_INFO("(Global) Exiting");
    return EXIT_SUCCESS;
}