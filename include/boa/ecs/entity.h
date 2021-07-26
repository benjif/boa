#ifndef BOA_ECS_ENTITY_H
#define BOA_ECS_ENTITY_H

#include "boa/utl/iteration.h"
#include "boa/ecs/component.h"
#include <cstdint>
#include <vector>
#include <bitset>
#include <algorithm>
#include <queue>
#include <optional>

namespace boa::ecs {

struct EntityMeta {
    EntityMeta(uint32_t id_)
        : id(id_),
          component_mask(COMPONENTS_START_COUNT)
    {
    }

    void grow_if_needed(uint32_t id);

    uint32_t id;
    std::vector<bool> component_mask;
};

struct EntityGroup {
    EntityGroup();
    static EntityGroup &get();

    uint32_t new_entity();
    void delete_entity(uint32_t e_id);

    void clear_entities();

    template <typename ...C>
    uint32_t copy_entity(uint32_t copy_e_id) {
        const EntityMeta &other = entities[copy_e_id];
        uint32_t c_ids[] = { component_id<C>()... };
        uint32_t c_size[] = { sizeof(C)... };

        uint32_t new_e_id = entities.size();

        if (free_entities.size() > 0) {
            new_e_id = free_entities.top();
            free_entities.pop();
        }

        for (size_t i = 0; i < sizeof...(C); i++) {
            if (!entities[copy_e_id].component_mask[c_ids[i]])
                continue;
            auto &component_store = ComponentStore::get();
            void *component_zone = component_store.get_component_zone_from_component_id(c_ids[i]);
            if (new_e_id * c_size[i] >= COMPONENT_ZONE_SIZE)
                throw std::runtime_error(fmt::format("Component overflow for entity {}", new_e_id));
            memcpy((char *)component_zone + c_size[i] * new_e_id, (char *)component_zone + c_size[i] * copy_e_id, c_size[i]);
        }

        EntityMeta new_entity_meta(new_e_id);
        new_entity_meta.component_mask = other.component_mask;
        entities.push_back(std::move(new_entity_meta));

        return new_e_id;
    }

    template <typename C>
    void enable(uint32_t e_id) {
        uint32_t c_id = component_id<C>();
        entities[e_id].grow_if_needed(c_id);
        entities[e_id].component_mask[c_id] = true;
    }

    template <typename C, typename ...Args>
    void make(uint32_t e_id, Args &&...args) {
        uint32_t c_id = component_id<C>();
        if (!has_component<C>(e_id))
            throw std::runtime_error("Attempted to make disabled component for entity");

        entities[e_id].grow_if_needed(c_id);
        auto &component_store = ComponentStore::get();
        C *component = component_store.get_component<C>(e_id);
        new(component) C(std::forward<Args>(args)...); 
    }

    template <typename C, typename ...Args>
    void enable_and_make(uint32_t e_id, Args &&...args) {
        uint32_t c_id = component_id<C>();
        entities[e_id].grow_if_needed(c_id);
        entities[e_id].component_mask[c_id] = true;
        auto &component_store = ComponentStore::get();
        C *component = component_store.get_component<C>(e_id);
        new(component) C(std::forward<Args>(args)...); 
    }

    template <typename C>
    void disable(uint32_t e_id) {
        uint32_t c_id = component_id<C>();
        entities[e_id].grow_if_needed(c_id);
        entities[e_id].component_mask[c_id] = false;
    }

    template <typename C>
    bool has_component(uint32_t e_id) const {
        uint32_t c_id = component_id<C>();
        if (c_id >= entities[e_id].component_mask.size())
            return false;
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

    template <typename ...C>
    std::optional<uint32_t> find_first_entity_with_component() const {
        uint32_t c_ids[] = { component_id<C>()... };
        for (auto &entity : entities) {
            if (std::all_of(std::begin(c_ids), std::end(c_ids), [&](uint32_t c_id) { return entity.component_mask[c_id]; }))
                return entity.id;
        }
        return std::nullopt;
    }

    uint32_t size() const {
        return entities.size();
    }

    std::vector<EntityMeta> entities;
    std::priority_queue<uint32_t> free_entities;
};

}

#endif
