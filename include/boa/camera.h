#ifndef BOA_CAMERA_H
#define BOA_CAMERA_H

#include <glm/glm.hpp>

namespace boa {

class Camera {
public:
    enum class DirectionFlags : int {
        Left        = 1 << 0,
        Right       = 1 << 1,
        Backward    = 1 << 2,
        Forward     = 1 << 3,
        //Up          = 1 << 4,
        //Down        = 1 << 5,
    };

    void update_position(float time_change, DirectionFlags directions);
    void update_target(const glm::dvec2 &cursor_offset);

    void set_sensitivity(float value);
    void set_movement_speed(float value);
    void set_position(const glm::vec3 &position);
    void set_target(const glm::vec3 &target);
    void set_up(const glm::vec3 &up);
    void set_pitch(float pitch);
    void set_yaw(float yaw);

    float get_sensitivity() const;
    float get_movement_speed() const;
    const glm::vec3 &get_position() const;
    const glm::vec3 &get_target() const;
    const glm::vec3 &get_up() const;
    float get_pitch() const;
    float get_yaw() const;

private:
    float m_sensitivity = 1.05f;
    float m_movement_speed = 0.08f;
    glm::vec3 m_position{ 0.0f, 0.0f, -2.0f };
    glm::vec3 m_target{ 0.0f, 0.0f, 1.0f };
    glm::vec3 m_up{ 0.0f, 1.0f, 0.0f };
    float m_pitch{ 0.0f }, m_yaw{ 90.0f };
};

}

inline boa::Camera::DirectionFlags operator|(boa::Camera::DirectionFlags lhs, boa::Camera::DirectionFlags rhs) {
    return static_cast<boa::Camera::DirectionFlags>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

inline boa::Camera::DirectionFlags operator&(boa::Camera::DirectionFlags lhs, boa::Camera::DirectionFlags rhs) {
    return static_cast<boa::Camera::DirectionFlags>(static_cast<int>(lhs) & static_cast<int>(rhs));
}

inline boa::Camera::DirectionFlags &operator|=(boa::Camera::DirectionFlags &lhs, boa::Camera::DirectionFlags rhs) {
    return (boa::Camera::DirectionFlags &)((int &)(lhs) |= static_cast<int>(rhs));
}

inline boa::Camera::DirectionFlags &operator&=(boa::Camera::DirectionFlags &lhs, boa::Camera::DirectionFlags rhs) {
    return (boa::Camera::DirectionFlags &)((int &)(lhs) &= static_cast<int>(rhs));
}

#endif