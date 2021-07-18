#ifndef BOA_GFX_ASSET_ANIMATION_H
#define BOA_GFX_ASSET_ANIMATION_H

#include "boa/macros.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include <stdint.h>
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
    void update(float time_change);
};

struct Animated {
    struct Animation {
        struct Component {
            struct SubComponent {
                float start_time, end_time;
                std::variant<glm::vec3, glm::quat> target_start;
                std::variant<glm::vec3, glm::quat> target_end;
            };

            std::vector<SubComponent> sub_components;
            size_t current_sub_component{ 0 };

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

            std::variant<glm::vec3, glm::quat> target_start;
            std::variant<glm::vec3, glm::quat> target_progress;

            uint32_t node_id;

            bool update(float time_change, float progress);
        };

        std::vector<Component> animation_components;

        bool update(float time_change, float progress);
    };

    std::vector<Animation> animations;

    bool active{ false };
    bool loop{ false };
    size_t active_animation;
    float progress{ 0.0f };

    void reset();
    void update(float time_change);
    glm::mat4 transform_for_node(size_t node_id);
};

}

#endif