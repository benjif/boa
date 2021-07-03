#ifndef BOA_INPUT_KEYBOARD_H
#define BOA_INPUT_KEYBOARD_H

#include <bitset>

namespace boa {

class Keyboard {
public:
    static void keyboard_callback(void *ptrs_v, int key, int scancode, int action, int mods);

    bool key(uint32_t key) const;
    void set_key(uint32_t key);
    void unset_key(uint32_t key);

private:
    std::bitset<348> m_keys;
};

}

#endif