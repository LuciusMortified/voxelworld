module;

#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

module vw.platform;

import std;
import vw.core;

namespace vw::plat {
namespace {

constexpr log::log_category lc{"window"};

auto as_glfw(void* handle) -> GLFWwindow* {
    return static_cast<GLFWwindow*>(handle);
}

}  // namespace

namespace detail {

struct window_callbacks {
    [[nodiscard]] static auto owner_of(GLFWwindow* handle) -> window* {
        return static_cast<window*>(glfwGetWindowUserPointer(handle));
    }

    static auto key(GLFWwindow* handle, int key, int scancode, int action, int mods) -> void {
        owner_of(handle)->on_key_(
            key, scancode, mods, action != GLFW_RELEASE, action == GLFW_REPEAT);
    }

    static auto mouse_button(GLFWwindow* handle, int button, int action, int mods) -> void {
        if (action == GLFW_PRESS || action == GLFW_RELEASE) {
            owner_of(handle)->on_mouse_button_(button, mods, action == GLFW_PRESS);
        }
    }

    static auto mouse_motion(GLFWwindow* handle, double pos_x, double pos_y) -> void {
        owner_of(handle)->on_mouse_move_(pos_x, pos_y);
    }

    static auto mouse_scroll(GLFWwindow* handle, double offset_x, double offset_y) -> void {
        owner_of(handle)->on_mouse_scroll_(offset_x, offset_y);
    }

    static auto resize(GLFWwindow* handle, int width, int height) -> void {
        owner_of(handle)->on_resize_(width, height);
    }

    static auto focus(GLFWwindow* handle, int focused) -> void {
        owner_of(handle)->on_focus_(focused == GLFW_TRUE);
    }

    static auto close(GLFWwindow* handle) -> void {
        owner_of(handle)->on_close_();
    }
};

}  // namespace detail

window::window(int32 width, int32 height, std::string_view title) : size_(width, height) {
    if (glfwInit() == GLFW_FALSE) {
        throw std::runtime_error("failed to initialize glfw");
    }

    log::info(lc, "GLFW {}", glfwGetVersionString());

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    handle_ = glfwCreateWindow(width, height, std::string(title).c_str(), nullptr, nullptr);
    if (handle_ == nullptr) {
        glfwTerminate();
        throw std::runtime_error("failed to create glfw window");
    }

    auto* handle = as_glfw(handle_);
    glfwSetWindowUserPointer(handle, this);

    glfwSetKeyCallback(handle, detail::window_callbacks::key);
    glfwSetMouseButtonCallback(handle, detail::window_callbacks::mouse_button);
    glfwSetCursorPosCallback(handle, detail::window_callbacks::mouse_motion);
    glfwSetScrollCallback(handle, detail::window_callbacks::mouse_scroll);
    glfwSetFramebufferSizeCallback(handle, detail::window_callbacks::resize);
    glfwSetWindowFocusCallback(handle, detail::window_callbacks::focus);
    glfwSetWindowCloseCallback(handle, detail::window_callbacks::close);
}

window::~window() {
    if (handle_ != nullptr) {
        glfwDestroyWindow(as_glfw(handle_));
    }
    glfwTerminate();
}

auto window::on_key_(int32 key, int32 scancode, int32 mods, bool pressed, bool repeat) -> void {
    const auto typed_key  = static_cast<keyboard::keys>(key);
    const auto typed_mods = static_cast<keyboard::mods>(mods);

    if (repeat) {
        key_repeat_event event(typed_key, scancode, typed_mods);
        event_dispatcher_.dispatch(event);
    } else if (pressed) {
        key_press_event event(typed_key, scancode, typed_mods);
        event_dispatcher_.dispatch(event);
    } else {
        key_release_event event(typed_key, scancode, typed_mods);
        event_dispatcher_.dispatch(event);
    }
}

auto window::on_mouse_button_(int32 button, int32 mods, bool pressed) -> void {
    const auto typed_button = static_cast<mouse::buttons>(button);
    const auto typed_mods   = static_cast<keyboard::mods>(mods);

    if (pressed) {
        mouse_press_event event(typed_button, typed_mods);
        event_dispatcher_.dispatch(event);
    } else {
        mouse_release_event event(typed_button, typed_mods);
        event_dispatcher_.dispatch(event);
    }
}

auto window::on_mouse_move_(float64 x, float64 y) -> void {
    last_cursor_pos_ = {x, y};

    mouse_move_event event(x, y);
    event_dispatcher_.dispatch(event);
}

auto window::on_mouse_scroll_(float64 offset_x, float64 offset_y) -> void {
    mouse_scroll_event event(offset_x, offset_y);
    event_dispatcher_.dispatch(event);
}

auto window::on_resize_(int32 width, int32 height) -> void {
    size_ = {width, height};

    window_resize_event event{width, height};
    event_dispatcher_.dispatch(event);
}

auto window::on_focus_(bool focused) -> void {
    window_focus_event event{focused};
    event_dispatcher_.dispatch(event);
}

auto window::on_close_() -> void {
    window_close_event event{};
    event_dispatcher_.dispatch(event);
}

auto window::should_close() const -> bool {
    return glfwWindowShouldClose(as_glfw(handle_)) == GLFW_TRUE;
}

auto window::poll_events() -> void {
    glfwPollEvents();
}

auto window::framebuffer_size() const -> vec2i {
    int width  = 0;
    int height = 0;
    glfwGetFramebufferSize(as_glfw(handle_), &width, &height);
    return {width, height};
}

auto window::create_surface(uint64 instance) const -> uint64 {
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    const VkResult result = glfwCreateWindowSurface(
        reinterpret_cast<VkInstance>(instance), as_glfw(handle_), nullptr, &surface);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to create vulkan surface");
    }

    return reinterpret_cast<uint64>(surface);
}

auto window::required_extensions() -> std::vector<const char*> {
    uint32 count               = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&count);

    return {glfw_extensions, glfw_extensions + count};
}

auto window::is_key_pressed(keyboard::keys key) const -> bool {
    return glfwGetKey(as_glfw(handle_), static_cast<int32>(key)) == GLFW_PRESS;
}

auto window::is_mouse_button_pressed(mouse::buttons button) const -> bool {
    return glfwGetMouseButton(as_glfw(handle_), static_cast<int32>(button)) == GLFW_PRESS;
}

auto window::get_cursor_pos() const -> vec2d {
    float64 x = 0.0;
    float64 y = 0.0;
    glfwGetCursorPos(as_glfw(handle_), &x, &y);
    return {x, y};
}

auto window::set_cursor_pos(vec2d pos) const -> void {
    glfwSetCursorPos(as_glfw(handle_), pos.x, pos.y);
    last_cursor_pos_ = pos;
}

auto window::set_cursor_pos(float64 x, float64 y) const -> void {
    set_cursor_pos({x, y});
}

auto window::set_cursor_mode(cursor_modes mode) const -> void {
    glfwSetInputMode(as_glfw(handle_), GLFW_CURSOR, static_cast<int32>(mode));
}

auto window::set_input_mode(input_modes mode, bool value) const -> void {
    glfwSetInputMode(as_glfw(handle_), static_cast<int32>(mode), value ? GLFW_TRUE : GLFW_FALSE);
}

auto window::set_title(std::string_view title) const -> void {
    glfwSetWindowTitle(as_glfw(handle_), std::string(title).c_str());
}

auto window::set_size(vec2i size) const -> void {
    glfwSetWindowSize(as_glfw(handle_), size.x, size.y);
    size_ = size;
}

auto window::set_size(int32 width, int32 height) const -> void {
    set_size({width, height});
}

auto window::set_position(vec2i pos) const -> void {
    glfwSetWindowPos(as_glfw(handle_), pos.x, pos.y);
}

auto window::set_position(int32 x, int32 y) const -> void {
    set_position({x, y});
}

auto window::maximize() const -> void {
    glfwMaximizeWindow(as_glfw(handle_));
}

auto window::minimize() const -> void {
    glfwIconifyWindow(as_glfw(handle_));
}

auto window::restore() const -> void {
    glfwRestoreWindow(as_glfw(handle_));
}

}  // namespace vw::plat
