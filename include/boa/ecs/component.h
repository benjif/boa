#ifndef BOA_ECS_COMPONENT_H
#define BOA_ECS_COMPONENT_H

#include "boa/macros.h"
#include <cstdint>
#include <memory>
#include <stdexcept>

namespace boa::ecs {

const size_t COMPONENTS_START_COUNT = 64;
//const size_t COMPONENT_ZONE_SIZE = 32 * KiB;
const size_t COMPONENT_ZONE_SIZE = 64 * KiB;
const uint32_t COMPONENTS_GROWTH_RATE = 2;

extern uint32_t component_type_count;

template <typename T>
uint32_t component_id();

class ComponentStore {
public:
    ComponentStore();
    static ComponentStore &get();

    template <typename Component>
    Component *get_component_zone() {
        uint32_t c_id = component_id<Component>();
        return (Component *)(m_component_store.get() + c_id * COMPONENT_ZONE_SIZE);
    }

    template <typename Component>
    Component *get_component(uint32_t e_id) {
        uint32_t c_id = component_id<Component>();
        if (e_id * sizeof(Component) >= COMPONENT_ZONE_SIZE)
            throw std::runtime_error(fmt::format("Component overflow for entity {}, component {}", e_id, c_id));
        Component *component_zone = get_component_zone<Component>();
        return &component_zone[e_id];
    }

    void *get_component_zone_from_component_id(uint32_t c_id) {
        return (void *)(m_component_store.get() + c_id * COMPONENT_ZONE_SIZE);
    }

    void grow();
    size_t max_components() const;

private:
    std::unique_ptr<char[]> m_component_store;
    uint32_t m_growth{ 1 };
};

template <typename T>
uint32_t component_id() {
    static uint32_t c_id = component_type_count++;
    auto &store = ComponentStore::get();
    if (c_id >= store.max_components())
        store.grow();
    return c_id;
}

}

#endif
