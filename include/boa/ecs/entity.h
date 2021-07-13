#ifndef BOA_ECS_ENTITY_H
#define BOA_ECS_ENTITY_H

#include "boa/iteration.h"
#include "boa/ecs/component.h"
#include <stdint.h>
#include <vector>
#include <bitset>
#include <algorithm>

namespace boa::ecs {

struct EntityMeta {
    EntityMeta(uint32_t id_)
        : id(id_)
    {
    }
    uint32_t id;
    std::bitset<MAX_COMPONENTS> component_mask;
};

// TODO: support more than one EntityGroup and ComponentStore?
struct EntityGroup {
    EntityGroup();
    static EntityGroup &get();

    uint32_t new_entity();

    template <typename C>
    void enable(uint32_t e_id) {
        uint32_t c_id = component_id<C>();
        entities[e_id].component_mask.set(c_id);
    }

    template <typename C>
    void disable(uint32_t e_id) {
        uint32_t c_id = component_id<C>();
        entities[e_id].component_mask.reset(c_id);
    }

    template <typename C>
    bool has_component(uint32_t e_id) {
        uint32_t c_id = component_id<C>();
        return entities[e_id].component_mask[c_id];
    }

    template <typename C>
    C &get_component(uint32_t e_id) {
        if (!has_component<C>(e_id))
            throw std::runtime_error("Attempted to get component that does not exist for entity");
        auto &component_store = ComponentStore::get();
        C *component_zone = component_store.get_component_zone<C>();
        return component_zone[e_id];
    }

    template <typename ...C, typename Callback>
    void for_each_entity_with_component(Callback callback) const {
        uint32_t c_ids[] = { component_id<C>()... };
        for (auto &entity : entities) {
            if (std::all_of(std::begin(c_ids), std::end(c_ids), [&](uint32_t c_id) { return entity.component_mask[c_id]; })) {
                if (callback(entity.id) == Iteration::Break)
                    break;
            }
        }
    }

    std::vector<EntityMeta> entities;
};

}

#endif