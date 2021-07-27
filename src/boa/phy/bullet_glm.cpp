#include "boa/phy/bullet_glm.h"

namespace boa::phy {

glm::vec3 bullet_to_glm(const btVector3 &v) {
    return glm::vec3(v.getX(), v.getY(), v.getZ());
}

btVector3 glm_to_bullet(const glm::vec3 &v) {
    return btVector3(v.x, v.y, v.z);
}

glm::quat bullet_to_glm(const btQuaternion &q) {
    return glm::quat(q.getW(), q.getX(), q.getY(), q.getZ());
}

btQuaternion glm_to_bullet(const glm::quat &q) {
    return btQuaternion(q.x, q.y, q.z, q.w);
}

btMatrix3x3 glm_to_bullet(const glm::mat3 &m) {
    return btMatrix3x3(m[0][0], m[1][0], m[2][0], m[0][1], m[1][1], m[2][1], m[0][2], m[1][2], m[2][2]);
}

btTransform glm_to_bullet(const glm::mat4 &m) {
    glm::mat3 m3(m);
    return btTransform(glm_to_bullet(m3), glm_to_bullet(glm::vec3(m[3][0], m[3][1], m[3][2])));
}

glm::mat4 bullet_to_glm(const btTransform &t) {
    glm::mat4 transform(1.0f);
    const btMatrix3x3& basis = t.getBasis();
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++)
            transform[j][i] = basis[i][j];
    }

    btVector3 origin = t.getOrigin();
    transform[3][0] = origin.getX();
    transform[3][1] = origin.getY();
    transform[3][2] = origin.getZ();
    transform[0][3] = 0.0f;
    transform[1][3] = 0.0f;
    transform[2][3] = 0.0f;
    transform[3][3] = 1.0f;

    return transform;
}

}
