#include "boa/macros.h"
#include "boa/iteration.h"
#include "boa/gfx/asset/gltf_model.h"
#include "boa/gfx/asset/asset_manager.h"
#include "boa/gfx/asset/animation_controller.h"
#include "boa/gfx/renderer.h"
#include "boa/ecs/ecs.h"
#include <chrono>

class BoaExampleApplication {
public:
    BoaExampleApplication()
        : renderer(asset_manager),
          window(renderer.get_window()),
          camera(renderer.get_camera()),
          keyboard(renderer.get_keyboard()),
          mouse (renderer.get_mouse())
    {
        /*sponza_entity = entity_group.new_entity();
        entity_group.enable<boa::ecs::Renderable>(sponza_entity);
        entity_group.enable<boa::ecs::Transformable>(sponza_entity);

        auto &sponza_transform_component = entity_group.get_component<boa::ecs::Transformable>(sponza_entity);
        {
            sponza_transform_component.orientation = glm::quat(0.0f, 0.0f, 0.0f, 0.0f);
            sponza_transform_component.translation = glm::vec3(0.0f, 0.0f, 0.0f);
            sponza_transform_component.scale = glm::vec3(1.0f, 1.0f, 1.0f);
            sponza_transform_component.update();
        }*/

        box_animated_entity = entity_group.new_entity();
        entity_group.enable<boa::ecs::Renderable>(box_animated_entity);
        entity_group.enable<boa::ecs::Transformable>(box_animated_entity);

        auto &box_transform_component = entity_group.get_component<boa::ecs::Transformable>(box_animated_entity);
        {
            box_transform_component.orientation = glm::quat(0.0f, 0.0f, 0.0f, 0.0f);
            box_transform_component.translation = glm::vec3(0.0f, 0.0f, 0.0f);
            box_transform_component.scale = glm::vec3(0.6f, 0.6f, 0.6f);
            box_transform_component.update();
        }
    }    

    void open_assets() {
        //sponza_model.open_gltf_file("models/sponza/glTF/Sponza.gltf");
        box_animated_model.open_gltf_file("models/BoxAnimated.gltf");
        //box_animated_model.open_gltf_file("models/animated_cube/AnimatedCube.gltf");
    }

    void load_models() {
        /*uint32_t sponza_model_id = asset_manager.load_model(renderer, sponza_model, "sponza");
        {
            auto &model_component = entity_group.get_component<boa::ecs::Renderable>(sponza_entity);
            model_component.model_id = sponza_model_id;
        }*/
        uint32_t box_animated_model_id = asset_manager.load_model(renderer, box_animated_model, "box_animated");
        {
            auto &model_component = entity_group.get_component<boa::ecs::Renderable>(box_animated_entity);
            model_component.model_id = box_animated_model_id;
        }
    }

    void load_animations() {
        animation_controller.load_animations(box_animated_model, box_animated_entity);
    }

    void load_skyboxes() {
        default_skybox = asset_manager.load_skybox(renderer, std::array<std::string, 6>{
            "skybox/mountains/front.jpg",
            "skybox/mountains/back.jpg",
            "skybox/mountains/top.jpg",
            "skybox/mountains/bottom.jpg",
            "skybox/mountains/right.jpg",
            "skybox/mountains/left.jpg",
            //"skybox/day/front.png",
            //"skybox/day/back.png",
            //"skybox/day/top.png",
            //"skybox/day/bottom.png",
            //"skybox/day/right.png",
            //"skybox/day/left.png",
        });

        renderer.set_active_skybox(default_skybox);
    }

    void play_animations() {
        animation_controller.play_animation(box_animated_entity, 0, true);
    }

    void setup_per_frame() {
        renderer.set_per_frame_callback([&](float time_change) {
            static float time = 0.0f;
            time += time_change;

            animation_controller.update(time_change);
            auto &box_transform_component = entity_group.get_component<boa::ecs::Transformable>(box_animated_entity);

            box_transform_component.translation.x = sin(time) / 8;
            box_transform_component.translation.y = 0.75f + cos(time) / 8;
            box_transform_component.update();

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
        renderer.run();
    }

private:
    boa::ecs::ComponentStore component_store;
    boa::ecs::EntityGroup entity_group;

    boa::gfx::AssetManager asset_manager;
    boa::gfx::AnimationController animation_controller;
    boa::gfx::Renderer renderer;

    uint32_t sponza_entity, box_animated_entity;
    uint32_t default_skybox;
    boa::gfx::glTFModel sponza_model, box_animated_model;

    boa::gfx::Window &window;
    boa::gfx::Camera &camera;
    boa::input::Keyboard &keyboard;
    boa::input::Mouse &mouse;
};

int main(int argc, char **argv) {
    LOG_INFO("(Global) Started");

    BoaExampleApplication app;

    app.open_assets();
    app.load_models();
    app.load_animations();
    app.load_skyboxes();

    app.setup_per_frame();
    app.play_animations();

    app.run();

    //boa::gfx::Skybox daytime = renderer.load_skybox("skybox/daytime.png");
    //renderer.set_skybox(daytime);

    LOG_INFO("(Global) Exiting");
    return EXIT_SUCCESS;
}