#include "boa/macros.h"
#include "boa/iteration.h"
#include "boa/gfx/linear.h"
#include "boa/gfx/lighting.h"
#include "boa/gfx/asset/gltf_model.h"
#include "boa/gfx/asset/asset_manager.h"
#include "boa/gfx/asset/animation.h"
#include "boa/gfx/renderer.h"
#include "boa/ecs/ecs.h"
#include <chrono>

class BoaExampleApplication {
public:
    BoaExampleApplication()
        : renderer(),
          window(renderer.get_window()),
          camera(renderer.get_camera()),
          keyboard(renderer.get_keyboard()),
          mouse(renderer.get_mouse()),
          asset_manager(renderer.get_asset_manager())
    {
        default_skybox = entity_group.new_entity();

        box_animated_entity = entity_group.new_entity();
        entity_group.enable<boa::gfx::Transformable>(box_animated_entity);
        entity_group.enable_and_make<boa::gfx::Transformable>(box_animated_entity,
            glm::quat{ 0.0f, 0.0f, 0.0f, 0.0f },
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 0.6f, 0.6f, 0.6f });
    }

    void load_assets() {
        box_animated_model.open_gltf_file("models/BoxAnimated.gltf");

        asset_manager.load_model(box_animated_entity, box_animated_model, "box_animated");
        animation_controller.load_animations(box_animated_entity, box_animated_model);

        uint32_t box_animated_entity2 =
            entity_group.copy_entity<boa::gfx::Transformable, boa::gfx::RenderableModel, boa::gfx::Animated>(box_animated_entity);

        auto &new_transform = entity_group.get_component<boa::gfx::Transformable>(box_animated_entity2);
        {
            new_transform.translation = glm::vec3(1.0f, 0.0f, 0.0f);
            new_transform.update();
        }

        animation_controller.play_animation(box_animated_entity2, 0, true);

        asset_manager.load_skybox(default_skybox, std::array<std::string, 6>{
            "skybox/mountains/right.png",
            "skybox/mountains/left.png",
            "skybox/mountains/top.png",
            "skybox/mountains/bottom.png",
            "skybox/mountains/front.png",
            "skybox/mountains/back.png",
            /*"skybox/day/right.png",
            "skybox/day/left.png",
            "skybox/day/top.png",
            "skybox/day/bottom.png",
            "skybox/day/front.png",
            "skybox/day/back.png",*/
        });

        asset_manager.set_active_skybox(default_skybox);
    }

    void load_lighting() {
        global_light = entity_group.new_entity();
        entity_group.enable_and_make<boa::gfx::GlobalLight>(global_light,
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 1.0f, 1.0f, 1.0f },
            glm::vec3{ 1.0f, 1.0f, 1.0f },
            glm::vec3{ 1.0f, 1.0f, 1.0f });
    }

    void play_looped_animations() {
        animation_controller.play_animation(box_animated_entity, 0, true);
    }

    void setup_per_frame() {
        renderer.set_per_frame_callback([&](float time_change) {
            static float time = 0.0f;
            time += time_change;

            auto &box_transform_component = entity_group.get_component<boa::gfx::Transformable>(box_animated_entity);

            box_transform_component.translation.x = sin(time) / 8;
            box_transform_component.translation.y = 0.75f + cos(time) / 8;
            box_transform_component.update();

            animation_controller.update(time_change);

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
    }

    void run() {
        window.show();
        renderer.run();
    }

private:
    boa::ecs::ComponentStore component_store;
    boa::ecs::EntityGroup entity_group;

    boa::gfx::Renderer renderer;

    boa::gfx::Window &window;
    boa::gfx::Camera &camera;
    boa::input::Keyboard &keyboard;
    boa::input::Mouse &mouse;

    boa::gfx::AssetManager &asset_manager;
    boa::gfx::AnimationController animation_controller;

    uint32_t sponza_entity, box_animated_entity;
    uint32_t default_skybox;
    uint32_t global_light;
    boa::gfx::glTFModel sponza_model, box_animated_model;
};

int main(int argc, char **argv) {
    LOG_INFO("(Global) Started");

    BoaExampleApplication app;
    app.load_assets();
    app.load_lighting();

    app.setup_per_frame();
    app.play_looped_animations();

    app.run();

    LOG_INFO("(Global) Exiting");
    return EXIT_SUCCESS;
}