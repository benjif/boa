#include "boa/utl/macros.h"
#include "boa/gfx/linear.h"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtx/matrix_decompose.hpp"

namespace boa::gfx {

void Transformable::update() {
    transform_matrix = glm::mat4(1.0f);
    transform_matrix *= glm::translate(translation);
    transform_matrix *= glm::toMat4(orientation);
    transform_matrix *= glm::scale(scale);
}

void Transformable::decompose() {
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(transform_matrix, scale, orientation, translation, skew, perspective);
}

glm::vec3 Box::center() const {
    return glm::vec3{
        (min.x + max.x) / 2,
        (min.y + max.y) / 2,
        (min.z + max.z) / 2,
    };
}

void Box::center_on_origin() {
    glm::vec3 box_center = center();
    min = min - box_center;
    max = max - box_center;
}

void Box::transform(const glm::mat4 &transform) {
    std::array<glm::vec3, 8> vertices = {
        min,
        max,
        glm::vec3{ min.x, min.y, max.z },
        glm::vec3{ min.x, max.y, min.z },
        glm::vec3{ max.x, min.y, min.z },
        glm::vec3{ min.x, max.y, max.z },
        glm::vec3{ max.x, min.y, max.z },
        glm::vec3{ max.x, max.y, min.z },
    };

    for (auto &vertex : vertices)
        vertex = transform * glm::vec4(vertex, 1.0f);

    glm::vec3 new_min, new_max;
    for (auto &vertex : vertices) {
        new_min = glm::min(vertex, new_min);
        new_max = glm::max(vertex, new_max);
    }

    min = std::move(new_min);
    max = std::move(new_max);
}

Sphere Sphere::bounding_sphere_from_bounding_box(const Box &box) {
    Sphere new_sphere;

    new_sphere.center = glm::vec3{
        (box.min.x + box.max.x) / 2,
        (box.min.y + box.max.y) / 2,
        (box.min.z + box.max.z) / 2,
    };

    new_sphere.radius = glm::distance(new_sphere.center, glm::vec3(box.min));

    return new_sphere;
}

// https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
void Frustum::update(glm::mat4 projection) {
    // left
    planes[0] = glm::vec4{
        projection[0].w + projection[0].x,
        projection[1].w + projection[1].x,
        projection[2].w + projection[2].x,
        projection[3].w + projection[3].x,
    };
    // right
    planes[1] = glm::vec4{
        projection[0].w - projection[0].x,
        projection[1].w - projection[1].x,
        projection[2].w - projection[2].x,
        projection[3].w - projection[3].x,
    };
    // top
    planes[2] = glm::vec4{
        projection[0].w - projection[0].y,
        projection[1].w - projection[1].y,
        projection[2].w - projection[2].y,
        projection[3].w - projection[3].y,
    };
    // bottom
    planes[3] = glm::vec4{
        projection[0].w + projection[0].y,
        projection[1].w + projection[1].y,
        projection[2].w + projection[2].y,
        projection[3].w + projection[3].y,
    };
    // near
    planes[4] = glm::vec4{
        projection[0].w + projection[0].z,
        projection[1].w + projection[1].z,
        projection[2].w + projection[2].z,
        projection[3].w + projection[3].z,
    };
    // far
    planes[5] = glm::vec4{
        projection[0].w - projection[0].z,
        projection[1].w - projection[1].z,
        projection[2].w - projection[2].z,
        projection[3].w - projection[3].z,
    };

    for (auto &plane : planes)
        plane /= glm::sqrt(plane.x * plane.x + plane.y * plane.y + plane.z * plane.z);
}

bool Frustum::is_box_within(const Box &box) const {
    TODO();
}

bool Frustum::is_sphere_within(const glm::vec3 &center, float radius) const {
    for (const auto &plane : planes) {
        if (plane.x * center.x + plane.y * center.y + plane.z * center.z + plane.w <= -radius)
            return false;
    }

    return true;
}

}
