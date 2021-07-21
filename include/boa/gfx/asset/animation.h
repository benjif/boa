#ifndef BOA_GFX_ASSET_ANIMATION_H
#define BOA_GFX_ASSET_ANIMATION_H

#include "boa/macros.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include <cstdint>
#include <variant>
#include <vector>

namespace boa::gfx {

class glTFModel;

class AnimationController {
    REMOVE_COPY_AND_ASSIGN(AnimationController);
public:
    AnimationController() {}

    void load_animations(uint32_t e_id, const boa::gfx::glTFModel &model);
    void play_animation(uint32_t e_id, uint32_t animation_id, bool loop = false);
    void stop_animation(uint32_t e_id);
    void update(float time_change);
};

struct Animated {
    struct Animation {
        struct Component {
            struct SubComponent {
                std::variant<glm::vec3, glm::quat> target_start;
                std::variant<glm::vec3, glm::quat> target_end;
                float start_time, end_time;
            };

            std::vector<SubComponent> sub_components;
            std::variant<glm::vec3, glm::quat> target_start;
            std::variant<glm::vec3, glm::quat> target_progress;

            uint32_t current_sub_component{ 0 };
            uint32_t node_id;

            enum class Type {
                Translation,
                Rotation,
                Scale,
                Weights,
            } type;

            enum class Interpolation {
                Linear,
                Step,
                CubicSpline,
            } interpolation;

            bool update(float time_change, float progress);
        };

        std::vector<Component> animation_components;

        bool update(float time_change, float progress);
    };

    std::vector<Animation> animations;

    float progress{ 0.0f };
    uint32_t active_animation;
    bool active{ false };
    bool loop{ false };

    void reset();
    void update(float time_change);
    glm::mat4 transform_for_node(size_t node_id);
};

}

#endif
