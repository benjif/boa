#include <iostream>
#include "boa/gfx/renderer.h"
#include "boa/input/mouse.h"

void boa::Mouse::mouse_button_callback(void *user_ptr_v, int button, int action, int mods) {
    auto user_pointers = reinterpret_cast<Renderer::WindowUserPointers *>(user_ptr_v);
    auto mouse = user_pointers->mouse;

    if (action == GLFW_PRESS)
        mouse->set_button(button);
    else if (action == GLFW_RELEASE)
        mouse->unset_button(button);
}

void boa::Mouse::cursor_callback(void *user_ptr_v, double x, double y) {
    auto user_pointers = reinterpret_cast<Renderer::WindowUserPointers *>(user_ptr_v);
    auto mouse = user_pointers->mouse;

    static bool first_mouse = true;
    if (first_mouse) {
        mouse->set_movement(glm::dvec2(0));
        mouse->set_position(glm::dvec2{ x, y });
        first_mouse = false;
        return;
    }

    mouse->set_movement(glm::dvec2{ x, y } - mouse->position());
    mouse->set_position(glm::dvec2{ x, y });
}

void boa::Mouse::scroll_callback(void *user_ptr_v, double x_offset, double y_offset) {
    auto user_pointers = reinterpret_cast<Renderer::WindowUserPointers *>(user_ptr_v);
    auto mouse = user_pointers->mouse;

    mouse->set_scroll_movement(glm::dvec2{ x_offset, y_offset });
}

glm::dvec2 boa::Mouse::position() const {
    return m_position;
}

glm::dvec2 boa::Mouse::last_movement() {
    glm::dvec2 tmp = m_movement;
    m_movement = { 0.0f, 0.0f };
    return tmp;
}

glm::dvec2 boa::Mouse::last_scroll_movement() {
    glm::dvec2 tmp = m_scroll_movement;
    m_scroll_movement = { 0.0f, 0.0f };
    return tmp;
}

void boa::Mouse::set_position(const glm::dvec2 &pos) {
    m_position = pos;
}

void boa::Mouse::set_movement(const glm::dvec2 &mov) {
    m_movement = mov;
}

void boa::Mouse::set_scroll_movement(const glm::dvec2 &mov) {
    m_scroll_movement = mov;
}

bool boa::Mouse::button(uint8_t button) const {
    assert(button < 8);
    return m_buttons[button];
}

void boa::Mouse::set_button(uint8_t button) {
    assert(button < 8);
    m_buttons[button] = true;
}

void boa::Mouse::unset_button(uint8_t button) {
    assert(button < 8);
    m_buttons[button] = false;
}