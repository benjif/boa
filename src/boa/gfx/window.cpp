#include "boa/gfx/window.h"
#include "GLFW/glfw3.h"

namespace boa::gfx {

Window::Window(int w, int h, const char *name)
    : m_width(w),
      m_height(h),
      m_window_name(name)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    m_window = glfwCreateWindow(m_width, m_height, m_window_name.c_str(), nullptr, nullptr);

    glfwSetWindowUserPointer(m_window, this);

    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(m_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    m_cursors[0] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    m_cursors[1] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
    m_cursors[2] = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);
    m_cursors[3] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
    m_cursors[4] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
    m_cursors[5] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
}

Window::~Window() {
    for (size_t i = 0; i < 6; i++)
        glfwDestroyCursor(m_cursors[i]);
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

GLFWwindow *Window::get_glfw_window() {
    return m_window;
}

void Window::poll_events() const {
    glfwPollEvents();
}

void Window::wait_events() const {
    glfwWaitEvents();
}

bool Window::should_close() const {
    return glfwWindowShouldClose(m_window);
}

void Window::set_should_close() const {
    glfwSetWindowShouldClose(m_window, 1);
}

void Window::show() const {
    glfwShowWindow(m_window);
}

void Window::hide() const {
    glfwHideWindow(m_window);
}

void Window::set_cursor(CursorShape shape) const {
    glfwSetCursor(m_window, m_cursors[static_cast<size_t>(shape)]);
}

void Window::set_cursor_disabled(bool hidden) {
    glfwSetInputMode(m_window, GLFW_CURSOR, hidden ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}

void Window::set_keyboard_callback(keyboard_callback_type callback) {
    auto wrap_f = +[](GLFWwindow *window, int key, int scancode, int action, int mods) {
        auto main_class = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
        auto callback = main_class->get_keyboard_callback();
        auto user_p = main_class->get_window_user_pointer();
        if (callback)
            callback(user_p, key, scancode, action, mods);
    };

    m_keyboard_callback = callback;
    glfwSetKeyCallback(m_window, wrap_f);
}

void Window::set_cursor_callback(cursor_callback_type callback) {
    auto wrap_f = +[](GLFWwindow *window, double x, double y) {
        auto main_class = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
        auto callback = main_class->get_cursor_callback();
        auto user_p = main_class->get_window_user_pointer();
        if (callback)
            callback(user_p, x, y);
    };

    m_cursor_callback = callback;
    glfwSetCursorPosCallback(m_window, wrap_f);
}

void Window::set_framebuffer_size_callback(resize_callback_type callback) {
    auto wrap_f = +[](GLFWwindow *window, int w, int h) {
        auto main_class = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
        auto callback = main_class->get_framebuffer_size_callback();
        auto user_p = main_class->get_window_user_pointer();
        if (callback)
            callback(user_p, w, h);
    };

    m_size_callback = callback;
    glfwSetFramebufferSizeCallback(m_window, wrap_f);
}

void Window::set_mouse_button_callback(mouse_button_callback_type callback) {
    auto wrap_f = +[](GLFWwindow *window, int button, int action, int mods) {
        auto main_class = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
        auto callback = main_class->get_mouse_button_callback();
        auto user_p = main_class->get_window_user_pointer();
        if (callback)
            callback(user_p, button, action, mods);
    };

    m_mouse_button_callback = callback;
    glfwSetMouseButtonCallback(m_window, wrap_f);
}

void Window::set_mouse_scroll_callback(mouse_scroll_callback_type callback) {
    auto wrap_f = +[](GLFWwindow *window, double x_offset, double y_offset) {
        auto main_class = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
        auto callback = main_class->get_mouse_scroll_callback();
        auto user_p = main_class->get_window_user_pointer();
        if (callback)
            callback(user_p, x_offset, y_offset);
    };

    m_mouse_scroll_callback = callback;
    glfwSetScrollCallback(m_window, wrap_f);
}

void Window::set_window_user_pointer(void *ptr) {
    m_user_pointer = ptr;
}

bool Window::get_cursor_disabled() const {
    return glfwGetInputMode(m_window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED;
}

Window::resize_callback_type Window::get_framebuffer_size_callback() const {
    return m_size_callback;
}

Window::keyboard_callback_type Window::get_keyboard_callback() const {
    return m_keyboard_callback;
}

Window::cursor_callback_type Window::get_cursor_callback() const {
    return m_cursor_callback;
}

Window::mouse_button_callback_type Window::get_mouse_button_callback() const {
    return m_mouse_button_callback;
}

Window::mouse_scroll_callback_type Window::get_mouse_scroll_callback() const {
    return m_mouse_scroll_callback;
}

void *Window::get_window_user_pointer() const {
    return m_user_pointer;
}

VkResult Window::create_window_surface(VkInstance instance, VkSurfaceKHR *surface) {
    return glfwCreateWindowSurface(instance, m_window, nullptr, surface);
}

void Window::get_framebuffer_size(int &w, int &h) {
    glfwGetFramebufferSize(m_window, &m_width, &m_height);
    w = m_width;
    h = m_height;
}

}
