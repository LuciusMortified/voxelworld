#pragma once

#ifndef VW_GFX_WINDOW_INL_H
#define VW_GFX_WINDOW_INL_H

#include <stdexcept>

namespace vw::gfx {

inline window::window(
    int width, int height, std::string_view title
)
    : size_(width, height) {
    if (!glfwInit()) {
        throw std::runtime_error("failed to initialize glfw");
    }

    const char* glfw_version_str = glfwGetVersionString();
    log::info(lc_, "GLFW {}", glfw_version_str);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window_ = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
    if (!window_) {
        glfwTerminate();
        throw std::runtime_error("failed to create glfw window");
    }

    glfwSetWindowUserPointer(window_, this);

    glfwSetKeyCallback(window_, key_callback);
    glfwSetMouseButtonCallback(window_, mouse_button_callback);
    glfwSetCursorPosCallback(window_, mouse_motion_callback);
    glfwSetScrollCallback(window_, mouse_scroll_callback);
    glfwSetFramebufferSizeCallback(window_, window_size_callback);
    glfwSetWindowFocusCallback(window_, window_focus_callback);
    glfwSetWindowCloseCallback(window_, window_close_callback);
}

inline window::~window() {
    if (window_) {
        glfwDestroyWindow(window_);
    }
    glfwTerminate();
}

inline void window::key_callback(
    GLFWwindow* wptr, int key, int scancode, int action, int mods
) {
    auto* w = static_cast<window*>(glfwGetWindowUserPointer(wptr));

    const auto keyboard_mods = static_cast<keyboard::mods>(mods);

    switch (action) {
        case GLFW_PRESS: {
            key_press_event event(static_cast<keyboard::keys>(key), scancode, keyboard_mods);
            w->event_dispatcher_.dispatch(event);
            break;
        }
        case GLFW_RELEASE: {
            key_release_event event(static_cast<keyboard::keys>(key), scancode, keyboard_mods);
            w->event_dispatcher_.dispatch(event);
            break;
        }
        case GLFW_REPEAT: {
            key_repeat_event event(static_cast<keyboard::keys>(key), scancode, keyboard_mods);
            w->event_dispatcher_.dispatch(event);
            break;
        }
        default:
            break;
    }
}

inline void window::mouse_button_callback(
    GLFWwindow* wptr, int button, int action, int mods
) {
    auto* w = static_cast<window*>(glfwGetWindowUserPointer(wptr));

    const auto keyboard_mods = static_cast<keyboard::mods>(mods);

    if (action == GLFW_PRESS) {
        mouse_press_event event(static_cast<mouse::buttons>(button), keyboard_mods);
        w->event_dispatcher_.dispatch(event);
    } else if (action == GLFW_RELEASE) {
        mouse_release_event event(static_cast<mouse::buttons>(button), keyboard_mods);
        w->event_dispatcher_.dispatch(event);
    }
}

inline void window::mouse_motion_callback(
    GLFWwindow* wptr, double pos_x, double pos_y
) {
    auto* w             = static_cast<window*>(glfwGetWindowUserPointer(wptr));
    w->last_cursor_pos_ = {pos_x, pos_y};

    mouse_move_event event(pos_x, pos_y);
    w->event_dispatcher_.dispatch(event);
}

inline void window::mouse_scroll_callback(
    GLFWwindow* wptr, double offset_x, double offset_y
) {
    auto* w = static_cast<window*>(glfwGetWindowUserPointer(wptr));

    mouse_scroll_event event(offset_x, offset_y);
    w->event_dispatcher_.dispatch(event);
}

inline void window::window_size_callback(
    GLFWwindow* wptr, int width, int height
) {
    auto* w  = static_cast<window*>(glfwGetWindowUserPointer(wptr));
    w->size_ = {width, height};

    window_resize_event event{width, height};
    w->event_dispatcher_.dispatch(event);
}

inline void window::window_focus_callback(
    GLFWwindow* wptr, int focused
) {
    auto* w = static_cast<window*>(glfwGetWindowUserPointer(wptr));

    window_focus_event event{focused == GLFW_TRUE};
    w->event_dispatcher_.dispatch(event);
}

inline void window::window_close_callback(
    GLFWwindow* wptr
) {
    auto* w = static_cast<window*>(glfwGetWindowUserPointer(wptr));

    window_close_event event{};
    w->event_dispatcher_.dispatch(event);
}

inline bool window::should_close() const {
    return glfwWindowShouldClose(window_);
}

inline void window::poll_events() {
    glfwPollEvents();
}

inline void window::get_framebuffer_size(
    int* width, int* height
) const {
    glfwGetFramebufferSize(window_, width, height);
}

inline VkSurfaceKHR window::create_surface(
    VkInstance instance
) {
    VkSurfaceKHR surface;
    VkResult result = glfwCreateWindowSurface(instance, window_, nullptr, &surface);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to create vulkan surface");
    }

    return surface;
}

inline std::vector<const char*> window::get_required_extensions() {
    uint32_t glfw_extension_count = 0;
    const char** glfw_extensions  = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    std::vector extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

#ifdef __APPLE__
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#endif

    return extensions;
}

inline bool window::is_key_pressed(
    keyboard::keys key
) const {
    return glfwGetKey(window_, static_cast<int>(key)) == GLFW_PRESS;
}

inline bool window::is_mouse_button_pressed(
    mouse::buttons button
) const {
    return glfwGetMouseButton(window_, static_cast<int>(button)) == GLFW_PRESS;
}

inline vec2d window::get_cursor_pos() const {
    double x, y;
    glfwGetCursorPos(window_, &x, &y);
    return {x, y};
}

inline void window::set_cursor_pos(
    vec2d pos
) const {
    glfwSetCursorPos(window_, pos.x, pos.y);
    last_cursor_pos_ = pos;
}

inline void window::set_cursor_pos(
    double x, double y
) const {
    set_cursor_pos({x, y});
}

inline void window::set_cursor_mode(
    cursor_modes mode
) const {
    glfwSetInputMode(window_, GLFW_CURSOR, static_cast<int>(mode));
}

inline void window::set_input_mode(
    input_modes mode, bool value
) const {
    glfwSetInputMode(window_, static_cast<int>(mode), value ? GLFW_TRUE : GLFW_FALSE);
}

inline void window::set_title(
    std::string_view title
) const {
    glfwSetWindowTitle(window_, title.data());
}

inline void window::set_size(
    vec2i size
) const {
    glfwSetWindowSize(window_, size.x, size.y);
    size_ = size;
}

inline void window::set_size(
    int width, int height
) const {
    set_size({width, height});
}

inline void window::set_position(
    vec2i pos
) const {
    glfwSetWindowPos(window_, pos.x, pos.y);
}

inline void window::set_position(
    int x, int y
) const {
    set_position({x, y});
}

inline void window::maximize() const {
    glfwMaximizeWindow(window_);
}

inline void window::minimize() const {
    glfwIconifyWindow(window_);
}

inline void window::restore() const {
    glfwRestoreWindow(window_);
}

}  // namespace vw::gfx

#endif  // VW_GFX_WINDOW_INL_H
