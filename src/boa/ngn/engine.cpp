#include "boa/utl/macros.h"
#include "boa/utl/iteration.h"
#include "boa/gfx/linear.h"
#include "boa/gfx/lighting.h"
#include "boa/gfx/asset/gltf_model.h"
#include "boa/gfx/asset/asset_manager.h"
#include "boa/gfx/asset/animation.h"
#include "boa/gfx/renderer.h"
#include "boa/phy/physics_controller.h"
#include "boa/ngn/object.h"
#include "boa/ngn/engine.h"
#include <chrono>

namespace boa::ngn {

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

    physics_controller.enable_debug_drawing(renderer);
    setup_input();

    //SystemManager::get().register_system<>();

    m_state.load_from_json("save/default.json");
    m_state.add_entities(asset_manager, animation_controller, physics_controller);
}

Engine::~Engine() {
    cleanup_interface();
}

void Engine::input_update(float time_change) {
    if (!ImGui::IsWindowFocused(ImGuiHoveredFlags_AnyWindow)) {
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
    }
}

void Engine::main_loop() {
#ifdef BENCHMARK
    auto render_start_time = std::chrono::high_resolution_clock::now();
#endif
    auto last_time = std::chrono::high_resolution_clock::now();
    auto current_time = last_time;

    while (!window.should_close()) {
        window.poll_events();

        current_time = std::chrono::high_resolution_clock::now();
        float time_change
            = std::chrono::duration<float, std::chrono::seconds::period>(current_time - last_time).count();
        last_time = current_time;

        draw_engine_interface();

        animation_controller.update(time_change);
        physics_controller.update(time_change);

        if (m_ui_state.show_physics_bounding_boxes) {
            physics_controller.debug_reset();
            physics_controller.debug_draw();
        }

        renderer.draw_frame();

        time_change *= 60.0f;
        input_update(time_change);

#ifdef BENCHMARK
        if (m_frame == BENCHMARK_FRAME_COUNT)
            break;
#endif
    }

#ifdef BENCHMARK
    auto render_stop_time = std::chrono::high_resolution_clock::now();
    float render_time = std::chrono::duration<float, std::chrono::seconds::period>(render_stop_time - render_start_time).count();
    fmt::print("Reached {} frames after {} seconds.\nAverage: {} FPS\n",
        BENCHMARK_FRAME_COUNT, render_time, BENCHMARK_FRAME_COUNT / render_time);
#endif

    renderer.wait_idle();
}

void Engine::deselect_object() {
    if (last_selected_entity.has_value()) {
        auto &ngn_config = entity_group.get_component<EngineSelectable>(last_selected_entity.value());
        ngn_config.selected = false;
        last_selected_entity.reset();
    }
}

void Engine::delete_entity(uint32_t e_id) {
    if (entity_group.has_component<boa::phy::Physical>(e_id))
        physics_controller.remove_entity(e_id);

    if (entity_group.has_component<boa::gfx::BaseRenderable>(e_id)) {
        entity_group.disable<boa::gfx::Renderable>(e_id);
    } else {
        entity_group.delete_entity(e_id);
    }
}

void Engine::setup_input() {
    keyboard.set_callback([&](int key, int action, int mods) {
        if (ImGui::IsWindowFocused(ImGuiHoveredFlags_AnyWindow)) {
            if (action == boa::ctl::Press && key == boa::ctl::KeyEscape)
                ImGui::SetWindowFocus(nullptr);
            else
                return;
        }

        static bool sticky_mouse_disabled = false;

        if (action == boa::ctl::Release) {
            if (key == boa::ctl::KeyLeftShift) {
                camera.set_movement_speed(0.08f);
                if (!sticky_mouse_disabled) {
                    window.set_cursor_disabled(false);
                    renderer.set_ui_mouse_enabled(true);
                }
            }
        } else if (action == boa::ctl::Press) {
            switch (key) {
            case boa::ctl::KeyEscape:
                m_dialog_shown = DialogShown::None;
                deselect_object();
                break;
            case boa::ctl::KeyLeftShift:
                camera.set_movement_speed(0.22f);
                window.set_cursor_disabled(true);
                renderer.set_ui_mouse_enabled(false);
                break;
            case boa::ctl::KeySpace:
                if (window.get_cursor_disabled()) {
                    sticky_mouse_disabled = false;
                    window.set_cursor_disabled(false);
                    renderer.set_ui_mouse_enabled(true);
                } else {
                    sticky_mouse_disabled = true;
                    window.set_cursor_disabled(true);
                    renderer.set_ui_mouse_enabled(false);
                }
                break;
            case boa::ctl::KeyDelete:
                if (last_selected_entity.has_value()) {
                    delete_entity(last_selected_entity.value());
                    last_selected_entity = std::nullopt;
                }
                break;
            case boa::ctl::Key1:
                m_ui_state.show_scene_properties = !m_ui_state.show_scene_properties;
                break;
            case boa::ctl::Key2:
                deselect_object();
                m_ui_state.show_object_properties = !m_ui_state.show_object_properties;
                break;
            case boa::ctl::Key3:
                m_ui_state.show_entity_create = !m_ui_state.show_entity_create;
                break;
            case boa::ctl::KeyL: {
                uint32_t new_light = entity_group.new_entity();
                entity_group.enable_and_make<boa::gfx::PointLight>(new_light,
                    glm::vec3(camera.get_position()),
                    glm::vec3{ 0.05f, 0.05f, 0.05f },
                    glm::vec3{ 0.4f, 0.4f, 0.4f },
                    glm::vec3{ 0.5f, 0.5f, 0.5f },
                    1.0f, 0.05f, 0.024f);
                break;
            }
            case boa::ctl::KeyG: {
                if (!last_selected_entity.has_value())
                    return;

                for (size_t i = 1; i < 300; i++) {
                    uint32_t entity_copy =
                        entity_group.copy_entity<boa::gfx::Renderable>(last_selected_entity.value());

                    entity_group.enable_and_make<boa::gfx::Transformable>(entity_copy,
                        glm::quat{ 0.0f, 0.0f, 1.0f, 0.0f },
                        glm::vec3{ sin(i * 0.23f), 20.0f + i * 5.0f, 0.0f },
                        glm::vec3{ 1.0f, 1.0f, 1.0f });
                    entity_group.enable_and_make<EngineSelectable>(entity_copy);

                    physics_controller.add_entity(entity_copy, i * 20.0f + 30.0f);
                }

                deselect_object();
                break;
            }
            case boa::ctl::KeyO:
                if (mods & boa::ctl::ModControl) {
                    m_dialog_shown = DialogShown::Open;
                }
                break;
            case boa::ctl::KeyP:
                if (physics_controller.is_physics_enabled()) {
                    physics_controller.disable_physics();
                    m_mode = EngineMode::Edit;
                } else {
                    physics_controller.enable_physics();
                    m_mode = EngineMode::Physics;
                }
                break;
            case boa::ctl::KeyS:
                if (mods & boa::ctl::ModControl) {
                    m_dialog_shown = DialogShown::Save;
                }
                break;
            }
        }
    });

    mouse.set_mouse_button_callback([&](int button, int action, int mods) {
        if (m_mode != EngineMode::Physics &&
                button == boa::ctl::MouseButtonLeft && action == boa::ctl::Press &&
                !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) &&
                !ImGuizmo::IsOver()) {
            deselect_object();

            glm::dvec2 mouse_position = mouse.get_position();
            auto hit = physics_controller.raycast_cursor_position(
                renderer.get_width(), renderer.get_height(),
                (uint32_t)mouse_position.x, (uint32_t)mouse_position.y,
                renderer.get_view_projection(),
                1000.0f);

            if (hit.has_value() && entity_group.has_component<EngineSelectable>(hit.value())) {
                auto &physical = entity_group.get_component<boa::phy::Physical>(hit.value());
                auto &ngn_config = entity_group.get_component<EngineSelectable>(hit.value());
                ngn_config.selected = true;
                last_selected_entity = hit.value();
                m_ui_state.show_object_properties = true;
            }
        }
    });
}

void Engine::run() {
    LOG_INFO("(Engine) Running");
    window.show();
    main_loop();
}

}
