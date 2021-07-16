#include "boa/macros.h"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/transform.hpp"
#include "boa/ecs/component_def.h"

namespace boa::ecs {

void Transformable::update() {
    transform_matrix = glm::mat4(1.0f);
    transform_matrix *= glm::translate(translation);
    transform_matrix *= glm::toMat4(orientation);
    transform_matrix *= glm::scale(scale);
}

bool Animated::Animation::Component::update(float time_change) {
    assert(interpolation == Interpolation::Linear);
    assert(type != Type::Weights);
    assert(parent != nullptr);

    if (parent->progress + time_change >= sub_components[current_sub_component].end_time) {
        if (current_sub_component + 1 >= sub_components.size())
            return false;
        current_sub_component++;
        sub_components[current_sub_component].target_start = target_progress;
    }

    const auto &active_sub_component = sub_components[current_sub_component];

    float sub_duration = active_sub_component.end_time - active_sub_component.start_time;
    float sub_progress = (parent->progress + time_change - active_sub_component.start_time) / sub_duration;

    switch (type) {
    case Type::Translation:
        target_progress = glm::mix(std::get<glm::vec3>(active_sub_component.target_start), std::get<glm::vec3>(active_sub_component.target_end), sub_progress);
        break;
    case Type::Rotation:
        target_progress = glm::slerp(std::get<glm::quat>(active_sub_component.target_start), std::get<glm::quat>(active_sub_component.target_end), sub_progress);
        break;
    case Type::Scale:
        target_progress = glm::mix(std::get<glm::vec3>(active_sub_component.target_start), std::get<glm::vec3>(active_sub_component.target_end), sub_progress);
        break;
    default:
        TODO();
    }

    return true;
}

bool Animated::Animation::update(float time_change) {
    bool still_working = false;
    for (auto &component : animation_components) {
        if (component.update(time_change))
            still_working = true;
    }

    parent->progress += time_change;
    return still_working;
}

void Animated::reset() {
    progress = 0.0f;
    for (auto &animation : animations) {
        for (auto &component : animation.animation_components) {
            component.current_sub_component = 0;
            switch (component.type) {
            case Animation::Component::Type::Translation:
            case Animation::Component::Type::Scale:
                component.target_progress = std::get<glm::vec3>(component.target_start);
                break;
            case Animation::Component::Type::Rotation:
                component.target_progress = std::get<glm::quat>(component.target_start);
                break;
            default:
                TODO();
            }

            for (auto &sub_component : component.sub_components) {
                switch (component.type) {
                case Animation::Component::Type::Translation:
                case Animation::Component::Type::Scale:
                    sub_component.target_start = std::get<glm::vec3>(component.target_start);
                    break;
                case Animation::Component::Type::Rotation:
                    sub_component.target_start = std::get<glm::quat>(component.target_start);
                    break;
                default:
                    TODO();
                }
            }
        }
    }
}

void Animated::update(float time_change) {
    if (!active)
        return;

    bool still_working = animations[active_animation].update(time_change);
    if (!still_working) {
        if (!loop)
            active = false;
        reset();
    }
}

glm::mat4 Animated::transform_for_node(size_t node_id) {
    if (!active)
        return glm::mat4(1.0f);

    glm::mat4 ret{ 1.0f };
    for (auto &component : animations[active_animation].animation_components) {
        if (node_id == component.node_id) {
            switch (component.type) {
            case Animation::Component::Type::Translation:
                ret *= glm::translate(std::get<glm::vec3>(component.target_progress));
                break;
            case Animation::Component::Type::Rotation:
                ret *= glm::toMat4(std::get<glm::quat>(component.target_progress));
                break;
            case Animation::Component::Type::Scale:
                ret *= glm::scale(std::get<glm::vec3>(component.target_progress));
                break;
            default:
                TODO();
            }
        }
    }

    return ret;
}

}