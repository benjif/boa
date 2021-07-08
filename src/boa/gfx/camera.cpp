#include <iostream>
#include "boa/gfx/camera.h"

namespace boa::gfx {

static inline bool check_dir(Camera::DirectionFlags directions, Camera::DirectionFlags dir) {
    return (directions & dir) == dir;
}

void Camera::update_position(float time_change, DirectionFlags directions) {
    if (check_dir(directions, DirectionFlags::Forward))
        m_position += time_change * m_movement_speed * m_target;
    if (check_dir(directions, DirectionFlags::Left))
        m_position -= glm::normalize(glm::cross(m_target, m_up)) * m_movement_speed * time_change;
    if (check_dir(directions, DirectionFlags::Backward))
        m_position -= time_change * m_movement_speed * m_target;
    if (check_dir(directions, DirectionFlags::Right))
        m_position += glm::normalize(glm::cross(m_target, m_up)) * m_movement_speed * time_change;
}

void Camera::update_target(const glm::dvec2 &cursor_offset) {
    m_pitch += -cursor_offset.y * m_sensitivity;
    m_yaw += cursor_offset.x * m_sensitivity;

    if (m_pitch > 89.0f)
        m_pitch = 89.0f;
    else if (m_pitch < -89.0f)
        m_pitch = -89.0f;

    glm::vec3 camera_direction{
        cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch)),
        sin(glm::radians(m_pitch)),
        sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch)),
    };

    m_target = glm::normalize(camera_direction);
}

void Camera::set_sensitivity(float value) {
    m_sensitivity = value;
}

void Camera::set_movement_speed(float value) {
    m_movement_speed = value;
}

void Camera::set_position(const glm::vec3 &position) {
    m_position = position;
}

void Camera::set_target(const glm::vec3 &target) {
    m_target = target;
}

void Camera::set_up(const glm::vec3 &up) {
    m_up = up;
}

void Camera::set_pitch(float pitch) {
    m_pitch = pitch;
}

void Camera::set_yaw(float yaw) {
    m_yaw = yaw;
}

float Camera::get_sensitivity() const {
    return m_sensitivity;
}

float Camera::get_movement_speed() const {
    return m_movement_speed;
}

const glm::vec3 &Camera::get_position() const {
    return m_position;
}

const glm::vec3 &Camera::get_target() const {
    return m_target;
}

const glm::vec3 &Camera::get_up() const {
    return m_up;
}

float Camera::get_pitch() const {
    return m_pitch;
}

float Camera::get_yaw() const {
    return m_yaw;
}

}