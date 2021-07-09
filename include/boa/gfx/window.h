#ifndef BOA_GFX_WINDOW_H
#define BOA_GFX_WINDOW_H

#include "boa/macros.h"
#include <vulkan/vulkan.hpp>
#include <string>

struct GLFWwindow;

namespace boa::gfx {

class Window {
    REMOVE_COPY_AND_ASSIGN(Window);
public:
    typedef void ((*resize_callback_type)(void *user_p, int w, int h));
    typedef void ((*keyboard_callback_type)(void *user_p, int key, int scancode, int action, int mods));
    typedef void ((*cursor_callback_type)(void *user_p, double x, double y));
    typedef void ((*mouse_button_callback_type)(void *user_p, int button, int action, int mods));
    typedef void ((*mouse_scroll_callback_type)(void *user_p, double x_offset, double y_offset));

    Window(int w, int h, const char *name);
    ~Window();

    GLFWwindow *get_glfw_window();

    void poll_events() const;
    bool should_close() const;
    void wait_events() const;

    void set_cursor_disabled(bool hidden);
    void set_window_user_pointer(void *ptr);
    void set_framebuffer_size_callback(resize_callback_type callback);
    void set_keyboard_callback(keyboard_callback_type callback);
    void set_cursor_callback(cursor_callback_type callback);
    void set_mouse_button_callback(mouse_button_callback_type callback);
    void set_mouse_scroll_callback(mouse_scroll_callback_type callback);

    bool get_cursor_disabled() const;
    resize_callback_type get_framebuffer_size_callback() const;
    keyboard_callback_type get_keyboard_callback() const;
    cursor_callback_type get_cursor_callback() const;
    mouse_button_callback_type get_mouse_button_callback() const;
    mouse_scroll_callback_type get_mouse_scroll_callback() const;

    void *get_window_user_pointer() const;
    void get_framebuffer_size(int &w, int &h);

    VkResult create_window_surface(VkInstance instance, VkSurfaceKHR *surface);

private:
    int m_width;
    int m_height;

    const std::string m_window_name;
    GLFWwindow *m_window;

    void *m_user_pointer;
    resize_callback_type m_size_callback;
    keyboard_callback_type m_keyboard_callback;
    cursor_callback_type m_cursor_callback;
    mouse_button_callback_type m_mouse_button_callback;
    mouse_scroll_callback_type m_mouse_scroll_callback;
};

}

#endif