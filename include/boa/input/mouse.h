#ifndef BOA_INPUT_MOUSE_H
#define BOA_INPUT_MOUSE_H

#include <glm/glm.hpp>
#include <bitset>

namespace boa::input {

class Mouse {
public:
    static void mouse_button_callback(void *user_ptr_v, int button, int action, int mods);
    static void cursor_callback(void *user_ptr_v, double x, double y);
    static void scroll_callback(void *user_ptr_v, double x_offset, double y_offset);

    glm::dvec2 get_position() const;
    glm::dvec2 last_movement();
    glm::dvec2 last_scroll_movement();
    void set_position(const glm::dvec2 &pos);
    void set_movement(const glm::dvec2 &mov);
    void set_scroll_movement(const glm::dvec2 &mov);

    bool button(uint8_t button) const;
    void set_button(uint8_t button);
    void unset_button(uint8_t button);

    bool is_first_move();
    void reset_first_move();

private:
    glm::dvec2 m_position;
    glm::dvec2 m_movement{ 0.0f, 0.0f };
    glm::dvec2 m_scroll_movement{ 0.0f, 0.0f };

    std::bitset<8> m_buttons;

    bool m_first_move{ true };
};

}

#endif
