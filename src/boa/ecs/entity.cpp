#include "boa/ecs/entity.h"

namespace boa::ecs {

EntityGroup *entity_group_instance = nullptr;

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

}