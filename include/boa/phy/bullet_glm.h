#ifndef BOA_PHY_BULLET_GLM_H
#define BOA_PHY_BULLET_GLM_H

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "LinearMath/btMatrix3x3.h"
#include "LinearMath/btVector3.h"
#include "LinearMath/btQuaternion.h"
#include "LinearMath/btTransform.h"

namespace boa::phy {

glm::vec3 bullet_to_glm(const btVector3 &v);
btVector3 glm_to_bullet(const glm::vec3 &v);
glm::quat bullet_to_glm(const btQuaternion &q);
btQuaternion glm_to_bullet(const glm::quat &q);
btMatrix3x3 glm_to_bullet(const glm::mat3 &m);
btTransform glm_to_bullet(const glm::mat4 &m);
glm::mat4 bullet_to_glm(const btTransform &t);

}

#endif
