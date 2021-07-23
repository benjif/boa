#ifndef BOA_CTL_MOUSE_H
#define BOA_CTL_MOUSE_H

#include "boa/ctl/action.h"
#include <glm/glm.hpp>
#include <bitset>

namespace boa::ctl {

enum MouseButton {
    MouseButton1            = 0,
    MouseButton2            = 1,
    MouseButton3            = 2,
    MouseButton4            = 3,
    MouseButton5            = 4,
    MouseButton6            = 5,
    MouseButton7            = 6,
    MouseButton8            = 7,
    MouseButtonLast         = MouseButton8,
    MouseButtonLeft         = MouseButton1,
    MouseButtonRight        = MouseButton2,
    MouseButtonMiddle       = MouseButton3,
};

class Mouse {
public:
    static void mouse_button_callback(void *user_ptr_v, int button, int action, int mods);
    static void cursor_callback(void *user_ptr_v, double x, double y);
    static void scroll_callback(void *user_ptr_v, double x_offset, double y_offset);

    void set_mouse_button_callback(std::function<void(int, int, int)> callback);

    glm::dvec2 get_position() const;
    glm::dvec2 last_movement();
    glm::dvec2 last_scroll_movement();
    bool button(uint8_t button) const;
    bool is_first_move();

private:
    glm::dvec2 m_position;
    glm::dvec2 m_movement{ 0.0f, 0.0f };
    glm::dvec2 m_scroll_movement{ 0.0f, 0.0f };

    bool m_first_move{ true };

    std::bitset<8> m_buttons;
    std::function<void(int, int, int)> m_mouse_button_callback;

    void set_button(uint8_t button);
    void unset_button(uint8_t button);
    void set_position(const glm::dvec2 &pos);
    void set_movement(const glm::dvec2 &mov);
    void set_scroll_movement(const glm::dvec2 &mov);

    void reset_first_move();
};

}

#endif
