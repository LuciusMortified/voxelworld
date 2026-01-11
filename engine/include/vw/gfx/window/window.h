#pragma once

#ifndef VW_GFX_WINDOW_H
#define VW_GFX_WINDOW_H

#include <string>
#include <string_view>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "vw/core.h"
#include "vw/log.h"
#include "vw/gfx/window/event.h"

namespace vw::gfx {
class window final {
public:
    window(int width, int height, std::string_view title);
    ~window();

    window(const window&)            = delete;
    window& operator=(const window&) = delete;

    window(window&&)            = delete;
    window& operator=(window&&) = delete;

    [[nodiscard]]
    bool should_close() const;

    void poll_events();
    void get_framebuffer_size(int* width, int* height) const;

    [[nodiscard]] VkSurfaceKHR create_surface(VkInstance instance);

    [[nodiscard]] static std::vector<const char*> get_required_extensions();

    [[nodiscard]]
    vec2i get_size() const {
        return size_;
    }

    [[nodiscard]]
    int get_width() const {
        return size_.x;
    }

    [[nodiscard]]
    int get_height() const {
        return size_.y;
    }

    [[nodiscard]]
    bool is_key_pressed(keyboard::keys key) const;

    [[nodiscard]]
    bool is_mouse_button_pressed(mouse::buttons button) const;

    [[nodiscard]]
    vec2d get_cursor_pos() const;

    void set_cursor_pos(vec2d pos) const;
    void set_cursor_pos(double x, double y) const;

    void set_cursor_mode(cursor_modes mode) const;
    void set_input_mode(input_modes mode, bool value) const;

    void set_title(std::string_view title) const;

    void set_size(vec2i size) const;
    void set_size(int width, int height) const;

    void set_position(vec2i pos) const;
    void set_position(int x, int y) const;

    void maximize() const;
    void minimize() const;
    void restore() const;

    GLFWwindow* get_handle() const {
        return window_;
    }

    template <event_type E, event_callback_type<E> F>
    event_sub<E> sub(
        F&& callback
    ) {
        return event_dispatcher_.sub<E>(std::forward<F>(callback));
    }

    template <event_type E>
    void unsub(
        event_sub<E> sub
    ) {
        return event_dispatcher_.unsub(sub);
    }

private:
    static void key_callback(GLFWwindow* wptr, int key, int scancode, int action, int mods);
    static void mouse_button_callback(GLFWwindow* wptr, int button, int action, int mods);
    static void mouse_motion_callback(GLFWwindow* wptr, double pos_x, double pos_y);
    static void mouse_scroll_callback(GLFWwindow* wptr, double offset_x, double offset_y);
    static void window_size_callback(GLFWwindow* wptr, int width, int height);
    static void window_focus_callback(GLFWwindow* wptr, int focused);
    static void window_close_callback(GLFWwindow* wptr);

    GLFWwindow* window_;
    std::string title_;

    mutable vec2i size_;
    mutable vec2d last_cursor_pos_ = {0.0, 0.0};

    event_dispatcher event_dispatcher_;

    static constexpr log::log_category lc_{"window"};
};
}  // namespace vw::gfx

#include "vw/gfx/window/window.inl.h"

#endif  // VW_GFX_WINDOW_H