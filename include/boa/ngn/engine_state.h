#ifndef BOA_NGN_ENGINE_STATE_H
#define BOA_NGN_ENGINE_STATE_H

#include "boa/gfx/asset/gltf_model.h"
#include "boa/gfx/linear.h"
#include "boa/gfx/lighting.h"
#include "boa/gfx/lighting_type.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include <vector>
#include <array>
#include <string>

namespace boa::gfx {
class AssetManager;
class AnimationController;
}

namespace boa::phy {
class PhysicsController;
}

namespace boa::ngn {

class EngineState {
public:
    static void save_to_json(boa::gfx::AssetManager &asset_manager,
                             boa::phy::PhysicsController &physics_controller,
                             const char *file_path);
    void load_from_json(const char *file_path);

    void add_entities(boa::gfx::AssetManager &asset_manager,
                      boa::gfx::AnimationController &animation_controller,
                      boa::phy::PhysicsController &physics_controller);

private:
    struct SavedRenderable {
        glm::vec3 translation{ 0.0f, 0.0f, 0.0f };
        glm::quat orientation{ 0.0f, 0.0f, 0.0f, 0.0f };
        glm::vec3 scale{ 1.0f, 1.0f, 1.0f };
        boa::gfx::LightingInteractivity lighting;
        bool engine_configurable{ true };
        uint32_t model;
        float mass;
    };

    struct SavedSkybox {
        SavedSkybox(std::array<std::string, 6> &&paths)
            : texture_paths(std::move(paths))
        {
        }
        std::array<std::string, 6> texture_paths;
    };

    uint32_t m_default_skybox{ 0 };
    std::vector<SavedRenderable> m_renderables;
    std::vector<SavedSkybox> m_skyboxes;
    std::vector<boa::gfx::glTFModel> m_models;
    std::vector<boa::gfx::GlobalLight> m_global_lights;
    std::vector<boa::gfx::PointLight> m_point_lights;
};

}

#endif
