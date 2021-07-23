#include "boa/ecs/component.h"

uint32_t boa::ecs::component_type_count = 0;

#include "boa/utl/macros.h"

namespace boa::ecs {

static ComponentStore *component_store_instance = nullptr;

ComponentStore::ComponentStore() {
    if (component_store_instance)
        return;
    m_component_store = std::make_unique<char[]>(COMPONENTS_START_COUNT * COMPONENT_ZONE_SIZE);
    component_store_instance = this;
}

ComponentStore &ComponentStore::get() {
    if (!component_store_instance)
        throw std::runtime_error("Attempted to get ComponentStore before construction");
    return *component_store_instance;
}

void ComponentStore::grow() {
    size_t old_size = m_growth * COMPONENTS_START_COUNT * COMPONENT_ZONE_SIZE;
    m_growth *= COMPONENTS_GROWTH_RATE;
    char *new_store = new char[m_growth * COMPONENTS_START_COUNT * COMPONENT_ZONE_SIZE];
    memcpy(new_store, m_component_store.get(), old_size);
    m_component_store.reset(new_store);
}

size_t ComponentStore::max_components() const {
    return m_growth * COMPONENTS_START_COUNT;
}

}
