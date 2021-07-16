#include "boa/ecs/ecs.h"
#include "boa/iteration.h"
#include "boa/gfx/asset/animation_controller.h"
#include "boa/gfx/asset/gltf_model.h"
#include "glm/gtc/type_ptr.hpp"

namespace boa::gfx {

void AnimationController::load_animations(const glTFModel &model, uint32_t e_id) {
    if (model.get_animation_count() == 0)
        return;

    auto &entity_group = ecs::EntityGroup::get();
    entity_group.enable<ecs::Animated>(e_id);

    auto &animated = entity_group.get_component<ecs::Animated>(e_id);
    animated.active = false;
    animated.loop = false;
    animated.progress = 0.0f;
    animated.animations.clear();

    model.for_each_animation([&](const auto &model_animation) {
        LOG_INFO("(Animation) Loading new animation for entity {}", e_id);

        ecs::Animated::Animation new_animation;
        new_animation.parent = &animated;
        new_animation.animation_components.clear();

        for (size_t i = 0; i < model_animation.channels.size(); i++) {
            const auto &channel = model_animation.channels.at(i);
            const auto &sampler = model_animation.samplers.at(channel.sampler);

            ecs::Animated::Animation::Component new_component;
            new_component.parent = &animated;
            new_component.current_sub_component = 0;
            new_component.sub_components.clear();
            new_component.node_id = channel.target.node;
            new_component.interpolation = static_cast<ecs::Animated::Animation::Component::Interpolation>(sampler.interpolation);
            new_component.type = static_cast<ecs::Animated::Animation::Component::Type>(channel.target.path);

            switch (new_component.type) {
            case ecs::Animated::Animation::Component::Type::Translation:
                new_component.target_start = new_component.target_progress = model.get_node(channel.target.node).translation;
                break;
            case ecs::Animated::Animation::Component::Type::Rotation:
                new_component.target_start = new_component.target_progress = model.get_node(channel.target.node).rotation;
                break;
            case ecs::Animated::Animation::Component::Type::Scale:
                new_component.target_start = new_component.target_progress = model.get_node(channel.target.node).scale;
                break;
            case ecs::Animated::Animation::Component::Type::Weights:
            default:
                TODO();
            }

            float global_time = 0.0f;
            for (size_t j = 0; j < sampler.in.size(); j++) {
                ecs::Animated::Animation::Component::SubComponent new_sub_component;
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

    entity_group.for_each_entity_with_component<boa::ecs::Animated>([&](uint32_t e_id) {
        auto &animated = entity_group.get_component<boa::ecs::Animated>(e_id);
        if (animated.active)
            animated.update(time_change);
        return Iteration::Continue;
    });
}

void AnimationController::play_animation(uint32_t e_id, uint32_t animation_id, bool loop) {
    auto &entity_group = boa::ecs::EntityGroup::get();

    if (!entity_group.has_component<boa::ecs::Animated>(e_id))
        return;

    auto &animated = entity_group.get_component<boa::ecs::Animated>(e_id);

    if (animation_id >= animated.animations.size())
        return;

    LOG_INFO("(Animation) Playing animation {} for entity {} (looped = {})", animation_id, e_id, loop);

    animated.active_animation = animation_id;
    animated.active = true;
    if (loop)
        animated.loop = true;
}

}