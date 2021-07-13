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

    auto &camera = renderer.get_camera();
    auto &keyboard = renderer.get_keyboard();
    auto &mouse = renderer.get_mouse();
    auto &window = renderer.get_window();

    renderer.set_per_frame_callback([&](float time_change) {
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