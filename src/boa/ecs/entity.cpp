#include "boa/ecs/entity.h"
#include <assert.h>

namespace boa::ecs {

EntityGroup *entity_group_instance = nullptr;

void EntityMeta::grow_component_mask(uint32_t id) {
    if (id >= component_mask.size())
        component_mask.resize(component_mask.size() * COMPONENTS_GROWTH_RATE);
    assert(component_mask.size() <= ComponentStore::get().size());
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
    entities.emplace_back(entities.size());
    return entities.size() - 1;
}

void EntityGroup::delete_entity(uint32_t e_id) {
    entities.erase(std::next(entities.begin(), e_id));
}

}