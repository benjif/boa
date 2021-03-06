#include "GLFW/glfw3.h"
#include "boa/gfx/renderer.h"
#include "boa/ctl/keyboard.h"

namespace boa::ctl {

void Keyboard::keyboard_callback(void *user_ptr_v, int key, int scancode, int action, int mods) {
    auto user_pointers = reinterpret_cast<gfx::Renderer::WindowUserPointers *>(user_ptr_v);
    auto keyboard = user_pointers->keyboard;
    auto keyboard_user_callback = keyboard->m_callback;

    if (keyboard_user_callback)
        keyboard_user_callback(key, action, mods);

    if (action == GLFW_PRESS) {
        keyboard->set_key(key, true);
    } else if (action == GLFW_RELEASE) {
        keyboard->set_key(key, false);
    }
}

void Keyboard::set_callback(std::function<void(int, int, int)> callback) {
    m_callback = callback;
}

void Keyboard::set_key(uint32_t key, bool pressed) {
    assert(key < 348);
    m_keys[key] = pressed;
}

bool Keyboard::key(uint32_t key) const {
    assert(key < 348);
    return m_keys[key];
}

void Keyboard::flip_key(uint32_t key) {
    assert(key < 348);
    m_keys[key] = !m_keys[key];
}

}
