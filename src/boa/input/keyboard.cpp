#include <GLFW/glfw3.h>
#include "boa/renderer.h"
#include "boa/input/keyboard.h"

void boa::Keyboard::keyboard_callback(void *user_ptr_v, int key, int scancode, int action, int mods) {
    auto user_pointers = reinterpret_cast<Renderer::WindowUserPointers *>(user_ptr_v);
    auto keyboard = user_pointers->keyboard;

    if (action == GLFW_PRESS) {
        keyboard->set_key(key);
    } else if (action == GLFW_RELEASE) {
        keyboard->unset_key(key);
    }
}

bool boa::Keyboard::key(uint32_t key) const {
    assert(key < 348);
    return m_keys[key];
}

void boa::Keyboard::set_key(uint32_t key) {
    assert(key < 348);
    m_keys[key] = true;
}

void boa::Keyboard::unset_key(uint32_t key) {
    assert(key < 348);
    m_keys[key] = false;
}