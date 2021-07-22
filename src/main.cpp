#include "boa/macros.h"
#include "boa/iteration.h"
#include "boa/gfx/linear.h"
#include "boa/gfx/lighting.h"
#include "boa/gfx/asset/gltf_model.h"
#include "boa/gfx/asset/asset_manager.h"
#include "boa/gfx/asset/animation.h"
#include "boa/gfx/renderer.h"
#include "boa/gfx/engine_interface.h"
#include "boa/phy/physics_controller.h"
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
        sponza_entity = entity_group.new_entity();
        entity_group.enable_and_make<boa::gfx::Transformable>(sponza_entity,
            glm::quat{ 0.0f, 0.0f, 0.0f, 0.0f },
            glm::vec3{ 0.0f, -10.0f, 0.0f },
            glm::vec3{ 1.0f, 1.0f, 1.0f });
        box_animated_entity = entity_group.new_entity();
        /*entity_group.enable_and_make<boa::gfx::Transformable>(box_animated_entity,
            glm::quat{ 0.0f, 0.0f, 0.0f, 0.0f },
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 0.6f, 0.6f, 0.6f });*/
        entity_group.enable_and_make<boa::gfx::Transformable>(box_animated_entity,
            glm::quat{ 0.0f, 0.0f, 0.0f, 0.0f },
            glm::vec3{ 0.0f, 20.0f, 0.0f },
            glm::vec3{ 1.0f, 1.0f, 1.0f });
    }

    void load_assets() {
        //box_animated_model.open_gltf_file("models/BoxAnimated.gltf");
        //sponza_model.open_gltf_file("models/sponza/Sponza.gltf");
        box_animated_model.open_gltf_file("models/domino_crown.gltf");
        sponza_model.open_gltf_file("models/primitives/plane.gltf");

        asset_manager.load_model(box_animated_entity, box_animated_model, boa::gfx::LightingInteractivity::BlinnPhong);
        asset_manager.load_model(sponza_entity, sponza_model, boa::gfx::LightingInteractivity::BlinnPhong);

        physics_controller.add_entity(box_animated_entity, 30.0f);
        physics_controller.add_entity(sponza_entity, 0.0f);

        //animation_controller.load_animations(box_animated_entity, box_animated_model);

        /*for (size_t i = 1; i < 32; i++) {
            uint32_t box_animated_entity_copy =
                entity_group.copy_entity<boa::gfx::Transformable, boa::gfx::RenderableModel, boa::gfx::Animated>(box_animated_entity);

            auto &new_transform = entity_group.get_component<boa::gfx::Transformable>(box_animated_entity_copy);
            {
                new_transform.translation = glm::vec3(i * 1.0f, 0.0f, 0.0f);
                new_transform.update();
            }

            animation_controller.play_animation(box_animated_entity_copy, 0, true);
        }*/

        asset_manager.load_skybox(default_skybox, std::array<std::string, 6>{
            "skybox/mountains/right.png",
            "skybox/mountains/left.png",
            "skybox/mountains/top.png",
            "skybox/mountains/bottom.png",
            "skybox/mountains/front.png",
            "skybox/mountains/back.png",
        });

        asset_manager.set_active_skybox(default_skybox);
    }

    void load_lighting() {
        global_light = entity_group.new_entity();
        entity_group.enable_and_make<boa::gfx::GlobalLight>(global_light,
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 0.3f, 0.3f, 0.3f },
            glm::vec3{ 0.5f, 0.5f, 0.5f },
            glm::vec3{ 0.1f, 0.1f, 0.1f });
        point_light = entity_group.new_entity();
        entity_group.enable_and_make<boa::gfx::PointLight>(point_light,
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 0.05f, 0.05f, 0.05f },
            glm::vec3{ 0.4f, 0.4f, 0.4f },
            glm::vec3{ 0.5f, 0.5f, 0.5f },
            0.4f, 0.07f, 0.031f);
        /*point_light = entity_group.new_entity();
        entity_group.enable_and_make<boa::gfx::PointLight>(point_light,
            glm::vec3{ 1.0f, 2.3f, 0.0f },
            glm::vec3{ 0.5f, 1.0f, 1.0f },
            glm::vec3{ 1.0f, 1.0f, 0.5f },
            glm::vec3{ 1.0f, 0.5f, 0.5f },
            1.0f, 0.09f, 0.032f);*/
    }

    void play_looped_animations() {
        //animation_controller.play_animation(box_animated_entity, 0, true);
    }

    void setup_per_frame() {
        renderer.set_per_frame_callback([&](float time_change) {
            static float time = 0.0f;
            time += time_change;

            auto &light_comp = entity_group.get_component<boa::gfx::PointLight>(point_light);
            light_comp.position = camera.get_position();

            animation_controller.update(time_change);
            physics_controller.update(time_change);

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

            boa::gfx::draw_engine_interface();
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
                } else if (key == boa::input::KEY_SPACE) {
                    uint32_t new_light = entity_group.new_entity();
                    entity_group.enable_and_make<boa::gfx::PointLight>(new_light,
                        glm::vec3(camera.get_position()),
                        glm::vec3{ 0.05f, 0.05f, 0.05f },
                        glm::vec3{ 0.4f, 0.4f, 0.4f },
                        glm::vec3{ 0.5f, 0.5f, 0.5f },
                        1.0f, 0.05f, 0.024f);
                } else if (key == boa::input::KEY_B) {
                    renderer.set_draw_bounding_boxes(!renderer.get_draw_bounding_boxes());
                } else if (key == boa::input::KEY_G) {
                    for (size_t i = 1; i < 8; i++) {
                        uint32_t box_copy =
                            entity_group.copy_entity<boa::gfx::RenderableModel>(box_animated_entity);

                        entity_group.enable_and_make<boa::gfx::Transformable>(box_copy,
                            glm::quat{ 0.0f, 0.0f, 0.0f, 0.0f },
                            glm::vec3{ sin(i * 0.23f), 20.0f + i * 5.0f, 0.0f },
                            glm::vec3{ 1.0f, 1.0f, 1.0f });

                        physics_controller.add_entity(box_copy, i * 20.0f + 10.0f);
                    }

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
    boa::phy::PhysicsController physics_controller;

    uint32_t sponza_entity, box_animated_entity;
    uint32_t default_skybox;
    uint32_t global_light, point_light;
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
