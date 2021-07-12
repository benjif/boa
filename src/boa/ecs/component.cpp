#include "boa/ecs/component.h"
#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"

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

void Transform::update() {
    transform_matrix = glm::mat4(1.0f);
    transform_matrix *= glm::translate(translation);
    //vk_model.transform_matrix = glm::rotate(vk_model.transform_matrix, entity_transform.orientation);
    transform_matrix *= glm::scale(scale);
}

}