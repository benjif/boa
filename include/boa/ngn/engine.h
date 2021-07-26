#ifndef BOA_NGN_ENGINE_H
#define BOA_NGN_ENGINE_H

#include "boa/ecs/ecs.h"
#include "boa/gfx/renderer.h"
#include "boa/gfx/asset/animation.h"
#include "boa/gfx/asset/gltf_model.h"
#include "boa/phy/physics_controller.h"
#include "boa/ngn/engine_state.h"
#include "imgui.h"
#include "ImGuizmo.h"

namespace boa::gfx {
class AssetManager;
class Window;
class Camera;
}

namespace boa::ctl {
class Keyboard;
class Mouse;
}

namespace boa::ngn {

class Engine {
public:
    Engine();
    void run();

private:
    EngineState m_state;

    struct UIState {
        bool show_scene_properties{ false };
        bool show_object_properties{ false };
        bool show_entity_create{ false };
    } m_ui_state;

    enum class EngineMode {
        Edit,
        Physics,
    } m_mode = EngineMode::Edit;

    enum class DialogShown {
        None,
        Save,
        Open,
        ImportModel,
    } m_dialog_shown;

    std::optional<uint32_t> last_selected_entity;

    boa::ecs::ComponentStore component_store;
    boa::ecs::EntityGroup entity_group;

    boa::gfx::Renderer renderer;

    boa::gfx::Window &window;
    boa::gfx::Camera &camera;
    boa::ctl::Keyboard &keyboard;
    boa::ctl::Mouse &mouse;

    boa::gfx::AssetManager &asset_manager;
    boa::gfx::AnimationController animation_controller;
    boa::phy::PhysicsController physics_controller;

    void setup_input();
    void input_update(float time_change);
    void deselect_object();

    void main_loop();

    void draw_engine_interface();
    void draw_main_menu_bar();
    void draw_scene_properties_window();
    void draw_object_properties_window();
    void draw_entity_create_window();
    void draw_global_lighting();
    void draw_toolbox();
    void draw_inactive_toolbox(ImGuizmo::OPERATION current_tool) const;
    void draw_physics_properties();
    void draw_inactive_physics_properties();
    void draw_animation();
    void draw_inactive_animation() const;
};

}

#endif
