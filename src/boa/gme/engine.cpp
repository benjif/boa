#include "boa/utl/macros.h"
#include "boa/utl/iteration.h"
#include "boa/gfx/linear.h"
#include "boa/gfx/lighting.h"
#include "boa/gfx/asset/gltf_model.h"
#include "boa/gfx/asset/asset_manager.h"
#include "boa/gfx/asset/animation.h"
#include "boa/gfx/renderer.h"
#include "boa/phy/physics_controller.h"
#include "boa/gme/object.h"
#include "boa/gme/engine.h"
#include "glm/gtc/type_ptr.hpp"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "imgui.h"
#include <chrono>

namespace boa::gme {

void Engine::draw_engine_interface() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    ImGui::Begin("Boa Window");

    static float ambient_lighting[3] = { 0.06f, 0.06f, 0.06f };
    static float diffuse_lighting[3] = { 0.6f, 0.6f, 0.6f };
    static float specular_lighting[3] = { 0.26f, 0.26f, 0.26f };
    static float direction[3] = { -0.2f, 1.0f, -0.3f };

    ImGui::BeginChild("Global Lighting");

    ImGui::ColorEdit3("Ambient", ambient_lighting);
    ImGui::ColorEdit3("Diffuse", diffuse_lighting);
    ImGui::ColorEdit3("Specular", specular_lighting);

    ImGui::InputFloat3("Direction", direction);

    ImGui::EndChild();

    auto &entity_group = boa::ecs::EntityGroup::get();
    auto g_light_e = entity_group.find_first_entity_with_component<boa::gfx::GlobalLight>();
    if (g_light_e.has_value()) {
        auto &g_light = entity_group.get_component<boa::gfx::GlobalLight>(g_light_e.value());
        g_light.direction = glm::make_vec3(direction);
        g_light.ambient = glm::make_vec3(ambient_lighting);
        g_light.diffuse = glm::make_vec3(diffuse_lighting);
        g_light.specular = glm::make_vec3(specular_lighting);
    }

    ImGui::End();
}

Engine::Engine()
    : renderer(),
      window(renderer.get_window()),
      camera(renderer.get_camera()),
      keyboard(renderer.get_keyboard()),
      mouse(renderer.get_mouse()),
      asset_manager(renderer.get_asset_manager()),
      physics_controller(asset_manager)
{
    LOG_INFO("(Engine) Initializing");

    default_skybox = entity_group.new_entity();
    baseplate_entity = entity_group.new_entity();
    crown_entity = entity_group.new_entity();
    entity_group.enable_and_make<boa::gfx::Transformable>(baseplate_entity,
        glm::quat{ 0.0f, 0.0f, 0.0f, 0.0f },
        glm::vec3{ 0.0f, -10.0f, 0.0f },
        glm::vec3{ 1.0f, 1.0f, 1.0f });
    entity_group.enable_and_make<boa::gfx::Transformable>(crown_entity,
        glm::quat{ 0.0f, 0.0f, 0.0f, 0.0f },
        glm::vec3{ 0.0f, 20.0f, 0.0f },
        glm::vec3{ 1.0f, 1.0f, 1.0f });

    entity_group.enable<EngineConfigurable>(crown_entity);
    entity_group.enable<EngineConfigurable>(baseplate_entity);

    load_assets();
    load_lighting();
    setup_input();
    setup_per_frame();
}

void Engine::load_assets() {
    crown_model.open_gltf_file("models/domino_crown.gltf");
    baseplate_model.open_gltf_file("models/primitives/plane.gltf");

    asset_manager.load_model(crown_entity, crown_model, boa::gfx::LightingInteractivity::BlinnPhong);
    asset_manager.load_model(baseplate_entity, baseplate_model, boa::gfx::LightingInteractivity::BlinnPhong);

    physics_controller.add_entity(crown_entity, 30.0f);
    physics_controller.add_entity(baseplate_entity, 0.0f);

    //animation_controller.load_animations(crown_entity, crown_model);
    /*for (size_t i = 1; i < 32; i++) {
        uint32_t crown_entity_copy =
            entity_group.copy_entity<boa::gfx::Transformable, boa::gfx::RenderableModel, boa::gfx::Animated>(crown_entity);

        auto &new_transform = entity_group.get_component<boa::gfx::Transformable>(crown_entity_copy);
        {
            new_transform.translation = glm::vec3(i * 1.0f, 0.0f, 0.0f);
            new_transform.update();
        }

        animation_controller.play_animation(crown_entity_copy, 0, true);
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

void Engine::load_lighting() {
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
}

void Engine::play_looped_animations() {
    //animation_controller.play_animation(crown_entity, 0, true);
}

void Engine::setup_per_frame() {
    renderer.set_per_frame_callback([&](float time_change) {
        auto &light_comp = entity_group.get_component<boa::gfx::PointLight>(point_light);
        light_comp.position = camera.get_position();

        animation_controller.update(time_change);
        physics_controller.update(time_change);

        time_change *= 60.0f;

        boa::gfx::Camera::DirectionFlags directions{};
        if (keyboard.key(boa::ctl::KeyW))
            directions |= boa::gfx::Camera::DirectionFlags::Forward;
        if (keyboard.key(boa::ctl::KeyA))
            directions |= boa::gfx::Camera::DirectionFlags::Left;
        if (keyboard.key(boa::ctl::KeyS))
            directions |= boa::gfx::Camera::DirectionFlags::Backward;
        if (keyboard.key(boa::ctl::KeyD))
            directions |= boa::gfx::Camera::DirectionFlags::Right;

        camera.update_position(time_change, directions);

        if (window.get_cursor_disabled()) {
            glm::dvec2 cursor_change = mouse.last_movement();
            camera.update_target(cursor_change);
        }

        draw_engine_interface();
    });
}

void Engine::setup_input() {
    keyboard.set_callback([&](int key, int action, int mods) {
        if (action == boa::ctl::Release) {
            if (key == boa::ctl::KeyLeftShift)
                camera.set_movement_speed(0.05f);
        } else if (action == boa::ctl::Press) {
            if (key == boa::ctl::KeyEscape) {
                if (window.get_cursor_disabled()) {
                    window.set_cursor_disabled(false);
                    renderer.set_ui_mouse_enabled(true);
                } else {
                    window.set_cursor_disabled(true);
                    renderer.set_ui_mouse_enabled(false);
                }
            } else if (key == boa::ctl::KeyLeftShift) {
                camera.set_movement_speed(0.22f);
            } else if (key == boa::ctl::KeySpace) {
                uint32_t new_light = entity_group.new_entity();
                entity_group.enable_and_make<boa::gfx::PointLight>(new_light,
                    glm::vec3(camera.get_position()),
                    glm::vec3{ 0.05f, 0.05f, 0.05f },
                    glm::vec3{ 0.4f, 0.4f, 0.4f },
                    glm::vec3{ 0.5f, 0.5f, 0.5f },
                    1.0f, 0.05f, 0.024f);
            } else if (key == boa::ctl::KeyB) {
                renderer.set_draw_bounding_boxes(!renderer.get_draw_bounding_boxes());
            } else if (key == boa::ctl::KeyG) {
                for (size_t i = 1; i < 300; i++) {
                    uint32_t crown_copy =
                        entity_group.copy_entity<boa::gfx::Renderable>(crown_entity);

                    entity_group.enable_and_make<boa::gfx::Transformable>(crown_copy,
                        glm::quat{ 0.0f, 0.0f, 1.0f, 0.0f },
                        glm::vec3{ sin(i * 0.23f), 20.0f + i * 5.0f, 0.0f },
                        glm::vec3{ 1.0f, 1.0f, 1.0f });
                    entity_group.enable_and_make<EngineConfigurable>(crown_copy);

                    physics_controller.add_entity(crown_copy, i * 20.0f + 30.0f);
                }
            }
        }
    });

    mouse.set_mouse_button_callback([&](int button, int action, int mods) {
        if (button == boa::ctl::MouseButtonLeft && action == boa::ctl::Press) {
            glm::dvec2 mouse_position = mouse.get_position();
            auto hit = physics_controller.raycast_cursor_position(
                renderer.get_width(), renderer.get_height(),
                (uint32_t)mouse_position.x, (uint32_t)mouse_position.y,
                renderer.get_view_projection(),
                1000.0f);

            if (last_selected_entity.has_value()) {
                auto &ngn_config = entity_group.get_component<EngineConfigurable>(last_selected_entity.value());
                ngn_config.selected = false;
            }

            last_selected_entity.reset();

            if (hit.has_value()) {
                LOG_INFO("Hit {}", hit.value());
                if (entity_group.has_component<EngineConfigurable>(hit.value())) {
                    auto &ngn_config = entity_group.get_component<EngineConfigurable>(hit.value());
                    ngn_config.selected = true;
                    last_selected_entity = hit.value();
                }
            }
        }
    });
}

void Engine::run() {
    LOG_INFO("(Engine) Running");
    play_looped_animations();
    window.show();
    renderer.run();
}

}
