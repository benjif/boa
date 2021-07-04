#ifndef BOA_INPUT_KEYBOARD_H
#define BOA_INPUT_KEYBOARD_H

#include <bitset>
#include <functional>

namespace boa {

class Keyboard {
public:
    static void keyboard_callback(void *ptrs_v, int key, int scancode, int action, int mods);

    void set_callback(std::function<void(int, int, int)> callback);

    bool key(uint32_t key) const;
    void flip_key(uint32_t key);
    void set_key(uint32_t key, bool pressed);

private:
    std::bitset<348> m_keys;
    std::function<void(int, int, int)> m_callback;
};

}

#endif