#include "boa/window.h"

boa::Window::Window(int w, int h, const char *name)
    : m_width(w),
      m_height(h),
      m_window_name(name)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    m_window = glfwCreateWindow(m_width, m_height, m_window_name.c_str(), nullptr, nullptr);

    glfwSetWindowUserPointer(m_window, this);

    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(m_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
}

boa::Window::~Window() {
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

GLFWwindow *boa::Window::get_glfw_window() {
    return m_window;
}

void boa::Window::poll_events() const {
    glfwPollEvents();
}

void boa::Window::wait_events() const {
    glfwWaitEvents();
}

bool boa::Window::should_close() const {
    return glfwWindowShouldClose(m_window);
}

void boa::Window::set_cursor_disabled(bool hidden) {
    glfwSetInputMode(m_window, GLFW_CURSOR, hidden ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}

void boa::Window::set_keyboard_callback(keyboard_callback_type callback) {
    auto wrap_f = +[](GLFWwindow *window, int key, int scancode, int action, int mods) {
        auto main_class = reinterpret_cast<boa::Window *>(glfwGetWindowUserPointer(window));
        auto callback = main_class->get_keyboard_callback();
        auto user_p = main_class->get_window_user_pointer();
        callback(user_p, key, scancode, action, mods);
    };

    m_keyboard_callback = callback;
    glfwSetKeyCallback(m_window, wrap_f);
}

void boa::Window::set_cursor_callback(cursor_callback_type callback) {
    auto wrap_f = +[](GLFWwindow *window, double x, double y) {
        auto main_class = reinterpret_cast<boa::Window *>(glfwGetWindowUserPointer(window));
        auto callback = main_class->get_cursor_callback();
        auto user_p = main_class->get_window_user_pointer();
        callback(user_p, x, y);
    };

    m_cursor_callback = callback;
    glfwSetCursorPosCallback(m_window, wrap_f);
}

void boa::Window::set_framebuffer_size_callback(resize_callback_type callback) {
    auto wrap_f = +[](GLFWwindow *window, int w, int h) {
        auto main_class = reinterpret_cast<boa::Window *>(glfwGetWindowUserPointer(window));
        auto callback = main_class->get_framebuffer_size_callback();
        auto user_p = main_class->get_window_user_pointer();
        callback(user_p, w, h);
    };

    m_size_callback = callback;
    glfwSetFramebufferSizeCallback(m_window, wrap_f);
}

void boa::Window::set_mouse_button_callback(mouse_button_callback_type callback) {
    auto wrap_f = +[](GLFWwindow *window, int button, int action, int mods) {
        auto main_class = reinterpret_cast<boa::Window *>(glfwGetWindowUserPointer(window));
        auto callback = main_class->get_mouse_button_callback();
        auto user_p = main_class->get_window_user_pointer();
        callback(user_p, button, action, mods);
    };

    m_mouse_button_callback = callback;
    glfwSetMouseButtonCallback(m_window, wrap_f);
}

void boa::Window::set_window_user_pointer(void *ptr) {
    m_user_pointer = ptr;
}

bool boa::Window::get_cursor_disabled() const {
    return glfwGetInputMode(m_window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED;
}

boa::Window::resize_callback_type boa::Window::get_framebuffer_size_callback() const {
    return m_size_callback;
}

boa::Window::keyboard_callback_type boa::Window::get_keyboard_callback() const {
    return m_keyboard_callback;
}

boa::Window::cursor_callback_type boa::Window::get_cursor_callback() const {
    return m_cursor_callback;
}

boa::Window::mouse_button_callback_type boa::Window::get_mouse_button_callback() const {
    return m_mouse_button_callback;
}

void *boa::Window::get_window_user_pointer() const {
    return m_user_pointer;
}

VkResult boa::Window::create_window_surface(VkInstance instance, VkSurfaceKHR *surface) {
    return glfwCreateWindowSurface(instance, m_window, nullptr, surface);
}

void boa::Window::get_framebuffer_size(int &w, int &h) {
    glfwGetFramebufferSize(m_window, &m_width, &m_height);
    w = m_width;
    h = m_height;
}