#include "boa/ecs/component_def.h"
#include "glm/gtx/transform.hpp"
#include "glm/gtx/quaternion.hpp"

namespace boa::ecs {

void Transform::update() {
    transform_matrix = glm::mat4(1.0f);
    transform_matrix *= glm::translate(translation);
    transform_matrix *= glm::toMat4(orientation);
    transform_matrix *= glm::scale(scale);
}

}