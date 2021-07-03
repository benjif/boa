#include <iostream>
#include "boa/camera.h"

static inline bool check_dir(boa::Camera::DirectionFlags directions, boa::Camera::DirectionFlags dir) {
    return (directions & dir) == dir;
}

void boa::Camera::update_position(float time_change, DirectionFlags directions) {
    if (check_dir(directions, DirectionFlags::Forward))
        m_position += time_change * m_movement_speed * m_target;
    if (check_dir(directions, DirectionFlags::Left))
        m_position -= glm::normalize(glm::cross(m_target, m_up)) * m_movement_speed * time_change;
    if (check_dir(directions, DirectionFlags::Backward))
        m_position -= time_change * m_movement_speed * m_target;
    if (check_dir(directions, DirectionFlags::Right))
        m_position += glm::normalize(glm::cross(m_target, m_up)) * m_movement_speed * time_change;
}

void boa::Camera::update_target(const glm::dvec2 &cursor_offset) {
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

void boa::Camera::set_sensitivity(float value) {
    m_sensitivity = value;
}

void boa::Camera::set_movement_speed(float value) {
    m_movement_speed = value;
}

void boa::Camera::set_position(const glm::vec3 &position) {
    m_position = position;
}

void boa::Camera::set_target(const glm::vec3 &target) {
    m_target = target;
}

void boa::Camera::set_up(const glm::vec3 &up) {
    m_up = up;
}

void boa::Camera::set_pitch(float pitch) {
    m_pitch = pitch;
}

void boa::Camera::set_yaw(float yaw) {
    m_yaw = yaw;
}

float boa::Camera::get_sensitivity() const {
    return m_sensitivity;
}

float boa::Camera::get_movement_speed() const {
    return m_movement_speed;
}

const glm::vec3 &boa::Camera::get_position() const {
    return m_position;
}

const glm::vec3 &boa::Camera::get_target() const {
    return m_target;
}

const glm::vec3 &boa::Camera::get_up() const {
    return m_up;
}

float boa::Camera::get_pitch() const {
    return m_pitch;
}

float boa::Camera::get_yaw() const {
    return m_yaw;
}