#ifndef BOA_ECS_COMPONENT_DEF_H
#define BOA_ECS_COMPONENT_DEF_H

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

namespace boa::ecs {

struct Transformable {
    glm::mat4 transform_matrix{ 1.0f };
    glm::vec3 translation{ 0.0f, 0.0f, 0.0f };
    glm::quat orientation{ 0.0f, 0.0f, 0.0f, 0.0f };
    glm::vec3 scale{ 1.0f, 1.0f, 1.0f };

    void update();
};

struct Renderable {
    uint32_t id;
};

}

#endif