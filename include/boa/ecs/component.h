#ifndef BOA_ECS_COMPONENT_H
#define BOA_ECS_COMPONENT_H

#include "boa/macros.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include <stdint.h>
#include <memory>
#include <stdexcept>

namespace boa::ecs {

const size_t MAX_COMPONENTS = 64;

struct Transform {
    glm::mat4 transform_matrix{ 1.0f };
    glm::vec3 translation{ 0.0f, 0.0f, 0.0f };
    glm::quat orientation{ 0.0f, 0.0f, 0.0f, 0.0f };
    glm::vec3 scale{ 1.0f, 1.0f, 1.0f };

    void update();
};

struct Model {
    uint32_t id;
};

extern uint32_t component_count;

template <typename T>
uint32_t component_id() {
    static uint32_t c_id = component_count++;
    if (c_id >= MAX_COMPONENTS)
        throw std::runtime_error("Component type overflow");
    return c_id;
}

class ComponentStore {
public:
    ComponentStore();
    static ComponentStore &get();

    template <typename Component>
    Component *get_component_zone() {
        uint32_t c_id = component_id<Component>();
        return (Component *)(m_component_store.data() + c_id * KiB);
    }

private:
    // 1 KiB per component type
    // component instance offset by entity id
    std::array<char, MAX_COMPONENTS * KiB> m_component_store;
};

}

#endif