#include "boa/ecs/entity.h"
#include <cassert>
#include <algorithm>

namespace boa::ecs {

EntityGroup *entity_group_instance = nullptr;

void EntityMeta::grow_if_needed(uint32_t id) {
    if (id >= component_mask.size())
        component_mask.resize(component_mask.size() * COMPONENTS_GROWTH_RATE);
    assert(component_mask.size() <= ComponentStore::get().max_components());
}

EntityGroup::EntityGroup() {
    if (entity_group_instance)
        return;
    else entity_group_instance = this;
}

EntityGroup &EntityGroup::get() {
    if (!entity_group_instance)
        throw std::runtime_error("Attempted to get EntityGroup before construction");
    return *entity_group_instance;
}

uint32_t EntityGroup::new_entity() {
    if (free_entities.size() > 0) {
        uint32_t free_ret = free_entities.back();
        free_entities.pop_back();
        entities[free_ret].active = true;
        return free_ret;
    }

    entities.emplace_back(entities.size());
    return entities.size() - 1;
}

void EntityGroup::delete_entity(uint32_t e_id) {
    if (e_id >= entities.size())
        return;
    entities[e_id].active = false;
    if (std::find(free_entities.cbegin(), free_entities.cend(), e_id) == free_entities.cend())
        free_entities.push_back(e_id);
}

void EntityGroup::clear_entities() {
    free_entities.clear();
    entities.clear();
}

}
