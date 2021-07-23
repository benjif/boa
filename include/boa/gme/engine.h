#ifndef BOA_GME_ENGINE_H
#define BOA_GME_ENGINE_H

#include "boa/ecs/ecs.h"
#include "boa/gfx/renderer.h"
#include "boa/gfx/asset/animation.h"
#include "boa/gfx/asset/gltf_model.h"
#include "boa/phy/physics_controller.h"

namespace boa::gfx {
class AssetManager;
class Window;
class Camera;
}

namespace boa::ctl {
class Keyboard;
class Mouse;
}

namespace boa::gme {

class Engine {
public:
    Engine();
    void run();

private:
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

    uint32_t baseplate_entity, crown_entity;
    uint32_t default_skybox;
    uint32_t global_light, point_light;
    boa::gfx::glTFModel baseplate_model, crown_model;

    void load_assets();
    void load_lighting();
    void play_looped_animations();
    void setup_per_frame();
    void setup_input();

    void draw_engine_interface();
};

}

#endif
