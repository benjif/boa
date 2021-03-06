#include <iostream>
#include "boa/gfx/renderer.h"
#include "boa/ctl/mouse.h"
#include "GLFW/glfw3.h"

namespace boa::ctl {

void Mouse::mouse_button_callback(void *user_ptr_v, int button, int action, int mods) {
    auto user_pointers = reinterpret_cast<boa::gfx::Renderer::WindowUserPointers *>(user_ptr_v);
    auto mouse = user_pointers->mouse;

    auto mouse_user_callback = mouse->m_mouse_button_callback;
    if (mouse_user_callback)
        mouse_user_callback(button, action, mods);

    if (action == GLFW_PRESS)
        mouse->set_button(button);
    else if (action == GLFW_RELEASE)
        mouse->unset_button(button);
}

void Mouse::cursor_callback(void *user_ptr_v, double x, double y) {
    auto user_pointers = reinterpret_cast<boa::gfx::Renderer::WindowUserPointers *>(user_ptr_v);
    auto mouse = user_pointers->mouse;

    if (mouse->is_first_move()) {
        mouse->set_movement(glm::dvec2(0));
        mouse->set_position(glm::dvec2{ x, y });
        return;
    }

    mouse->set_movement(glm::dvec2{ x, y } - mouse->get_position());
    mouse->set_position(glm::dvec2{ x, y });
}

void Mouse::scroll_callback(void *user_ptr_v, double x_offset, double y_offset) {
    auto user_pointers = reinterpret_cast<boa::gfx::Renderer::WindowUserPointers *>(user_ptr_v);
    auto mouse = user_pointers->mouse;

    mouse->set_scroll_movement(glm::dvec2{ x_offset, y_offset });
}

void Mouse::set_mouse_button_callback(std::function<void(int, int, int)> callback) {
    m_mouse_button_callback = callback;
}

glm::dvec2 Mouse::get_position() const {
    return m_position;
}

glm::dvec2 Mouse::last_movement() {
    glm::dvec2 tmp = m_movement;
    m_movement = { 0.0f, 0.0f };
    return tmp;
}

glm::dvec2 Mouse::last_scroll_movement() {
    glm::dvec2 tmp = m_scroll_movement;
    m_scroll_movement = { 0.0f, 0.0f };
    return tmp;
}

void Mouse::set_position(const glm::dvec2 &pos) {
    m_position = pos;
}

void Mouse::set_movement(const glm::dvec2 &mov) {
    m_movement = mov;
}

void Mouse::set_scroll_movement(const glm::dvec2 &mov) {
    m_scroll_movement = mov;
}

bool Mouse::button(uint8_t button) const {
    assert(button < 8);
    return m_buttons[button];
}

void Mouse::set_button(uint8_t button) {
    assert(button < 8);
    m_buttons[button] = true;
}

void Mouse::unset_button(uint8_t button) {
    assert(button < 8);
    m_buttons[button] = false;
}

bool Mouse::is_first_move() {
    bool tmp = m_first_move;
    m_first_move = false;
    return tmp;
}

void Mouse::reset_first_move() {
    m_first_move = true;
}

}
