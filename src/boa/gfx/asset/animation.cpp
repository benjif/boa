#include "boa/ecs/ecs.h"
#include "boa/iteration.h"
#include "boa/gfx/asset/animation.h"
#include "boa/gfx/asset/gltf_model.h"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/transform.hpp"

namespace boa::gfx {

void AnimationController::load_animations(uint32_t e_id, const glTFModel &model) {
    if (model.get_animation_count() == 0)
        return;

    auto &entity_group = ecs::EntityGroup::get();
    entity_group.enable<Animated>(e_id);

    auto &animated = entity_group.get_component<Animated>(e_id);
    animated.active = false;
    animated.loop = false;
    animated.progress = 0.0f;
    animated.animations.clear();

    model.for_each_animation([&](const auto &model_animation) {
        LOG_INFO("(Animation) Loading new animation for entity {}", e_id);

        Animated::Animation new_animation;
        new_animation.animation_components.clear();

        for (size_t i = 0; i < model_animation.channels.size(); i++) {
            const auto &channel = model_animation.channels.at(i);
            const auto &sampler = model_animation.samplers.at(channel.sampler);

            Animated::Animation::Component new_component;
            new_component.current_sub_component = 0;
            new_component.node_id = channel.target.node;
            new_component.interpolation = static_cast<Animated::Animation::Component::Interpolation>(sampler.interpolation);
            new_component.type = static_cast<Animated::Animation::Component::Type>(channel.target.path);

            switch (new_component.type) {
            case Animated::Animation::Component::Type::Translation:
                new_component.target_start = new_component.target_progress = model.get_node(channel.target.node).translation;
                break;
            case Animated::Animation::Component::Type::Rotation:
                new_component.target_start = new_component.target_progress = model.get_node(channel.target.node).rotation;
                break;
            case Animated::Animation::Component::Type::Scale:
                new_component.target_start = new_component.target_progress = model.get_node(channel.target.node).scale;
                break;
            case Animated::Animation::Component::Type::Weights:
            default:
                TODO();
            }

            float global_time = 0.0f;
            for (size_t j = 0; j < sampler.in.size(); j++) {
                Animated::Animation::Component::SubComponent new_sub_component;
                new_sub_component.start_time = global_time;
                new_sub_component.end_time = global_time + sampler.in[j];

                switch (channel.target.path) {
                case glTFModel::AnimationChannel::Path::Translation:
                    new_sub_component.target_end = glm::make_vec3(sampler.out.data() + 3 * j);
                    new_sub_component.target_start = model.get_node(channel.target.node).translation;
                    break;
                case glTFModel::AnimationChannel::Path::Rotation:
                    new_sub_component.target_end = glm::make_quat(sampler.out.data() + 4 * j);
                    new_sub_component.target_start = model.get_node(channel.target.node).rotation;
                    break;
                case glTFModel::AnimationChannel::Path::Scale:
                    new_sub_component.target_end = glm::make_vec3(sampler.out.data() + 3 * j);
                    new_sub_component.target_start = model.get_node(channel.target.node).scale;
                    break;
                case glTFModel::AnimationChannel::Path::Weights:
                default:
                    TODO();
                }

                new_component.sub_components.push_back(std::move(new_sub_component));
                global_time += sampler.in[j];
            }

            new_animation.animation_components.push_back(std::move(new_component));
        }

        animated.animations.push_back(std::move(new_animation));
        return Iteration::Continue;
    });
}

void AnimationController::update(float time_change) {
    auto &entity_group = boa::ecs::EntityGroup::get();

    entity_group.for_each_entity_with_component<Animated>([&](uint32_t e_id) {
        auto &animated = entity_group.get_component<Animated>(e_id);
        if (animated.active)
            animated.update(time_change);
        return Iteration::Continue;
    });
}

void AnimationController::play_animation(uint32_t e_id, uint32_t animation_id, bool loop) {
    auto &entity_group = boa::ecs::EntityGroup::get();

    if (!entity_group.has_component<Animated>(e_id))
        return;

    auto &animated = entity_group.get_component<Animated>(e_id);

    if (animation_id >= animated.animations.size())
        return;

    LOG_INFO("(Animation) Playing animation {} for entity {} (looped = {})", animation_id, e_id, loop);

    animated.active_animation = animation_id;
    animated.active = true;
    animated.loop = loop;
}

bool Animated::Animation::Component::update(float time_change, float progress) {
    assert(interpolation == Interpolation::Linear);
    assert(type != Type::Weights);

    while (progress + time_change >= sub_components[current_sub_component].end_time) {
        if (current_sub_component + 1 >= sub_components.size())
            return false;
        current_sub_component++;
        sub_components[current_sub_component].target_start = target_progress;
    }

    const auto &active_sub_component = sub_components[current_sub_component];

    float sub_duration = active_sub_component.end_time - active_sub_component.start_time;
    float sub_progress = (progress + time_change - active_sub_component.start_time) / sub_duration;

    assert(sub_progress >= 0.0f);
    assert(sub_progress <= 1.0f);

    switch (type) {
    case Type::Scale:
    case Type::Translation:
        target_progress = glm::mix(std::get<glm::vec3>(active_sub_component.target_start), std::get<glm::vec3>(active_sub_component.target_end), sub_progress);
        break;
    case Type::Rotation:
        target_progress = glm::slerp(std::get<glm::quat>(active_sub_component.target_start), std::get<glm::quat>(active_sub_component.target_end), sub_progress);
        break;
    default:
        TODO();
    }

    return true;
}

bool Animated::Animation::update(float time_change, float progress) {
    bool still_working = false;
    for (auto &component : animation_components) {
        if (component.update(time_change, progress))
            still_working = true;
    }

    return still_working;
}

void Animated::reset() {
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
    progress = 0.0f;
}

void Animated::update(float time_change) {
    if (!active)
        return;

    bool still_working = animations[active_animation].update(time_change, progress);
    progress += time_change;
    if (!still_working) {
        if (!loop)
            active = false;
        reset();
    }
}

glm::mat4 Animated::transform_for_node(size_t node_id) {
    if (!active)
        return glm::mat4{ 1.0f };

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