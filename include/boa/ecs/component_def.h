#ifndef BOA_ECS_COMPONENT_DEF_H
#define BOA_ECS_COMPONENT_DEF_H

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include <vector>
#include <variant>

namespace boa::ecs {

struct Transformable {
    glm::mat4 transform_matrix{ 1.0f };
    glm::vec3 translation{ 0.0f, 0.0f, 0.0f };
    glm::quat orientation{ 0.0f, 0.0f, 0.0f, 0.0f };
    glm::vec3 scale{ 1.0f, 1.0f, 1.0f };

    void update();
};

struct Renderable {
    uint32_t model_id;
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
            Animated *parent;

            bool update(float time_change);
        };

        std::vector<Component> animation_components;
        Animated *parent;

        bool update(float time_change);
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