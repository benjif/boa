#include "boa/ecs/component.h"

uint32_t boa::ecs::component_count = 0;

namespace boa::ecs {

static ComponentStore *component_store_instance = nullptr;

ComponentStore::ComponentStore() {
    if (component_store_instance)
        return;
    component_store_instance = this;
}

ComponentStore &ComponentStore::get() {
    if (!component_store_instance)
        throw std::runtime_error("Attempted to get ComponentStore before construction");
    return *component_store_instance;
}

}