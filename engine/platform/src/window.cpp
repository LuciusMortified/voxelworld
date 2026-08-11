module;

#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

module vw.platform;

import std;
import :logging;

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

    static void key(GLFWwindow* handle, int key, int scancode, int action, int mods) {
        owner_of(handle)->on_key_(
            key, scancode, mods, action != GLFW_RELEASE, action == GLFW_REPEAT);
    }

    static void mouse_button(GLFWwindow* handle, int button, int action, int mods) {
        if (action == GLFW_PRESS || action == GLFW_RELEASE) {
            owner_of(handle)->on_mouse_button_(button, mods, action == GLFW_PRESS);
        }
    }

    static void mouse_motion(GLFWwindow* handle, double pos_x, double pos_y) {
        owner_of(handle)->on_mouse_move_(pos_x, pos_y);
    }

    static void mouse_scroll(GLFWwindow* handle, double offset_x, double offset_y) {
        owner_of(handle)->on_mouse_scroll_(offset_x, offset_y);
    }

    static void resize(GLFWwindow* handle, int width, int height) {
        owner_of(handle)->on_resize_(width, height);
    }

    static void focus(GLFWwindow* handle, int focused) {
        owner_of(handle)->on_focus_(focused == GLFW_TRUE);
    }

    static void close(GLFWwindow* handle) {
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

void window::on_key_(int32 key, int32 scancode, int32 mods, bool pressed, bool repeat) {
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

void window::on_mouse_button_(int32 button, int32 mods, bool pressed) {
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

void window::on_mouse_move_(float64 x, float64 y) {
    last_cursor_pos_ = {x, y};

    mouse_move_event event(x, y);
    event_dispatcher_.dispatch(event);
}

void window::on_mouse_scroll_(float64 offset_x, float64 offset_y) {
    mouse_scroll_event event(offset_x, offset_y);
    event_dispatcher_.dispatch(event);
}

void window::on_resize_(int32 width, int32 height) {
    size_ = {width, height};

    window_resize_event event{width, height};
    event_dispatcher_.dispatch(event);
}

void window::on_focus_(bool focused) {
    window_focus_event event{focused};
    event_dispatcher_.dispatch(event);
}

void window::on_close_() {
    window_close_event event{};
    event_dispatcher_.dispatch(event);
}

auto window::should_close() const -> bool {
    return glfwWindowShouldClose(as_glfw(handle_)) == GLFW_TRUE;
}

void window::poll_events() {
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

void window::set_cursor_pos(vec2d pos) const {
    glfwSetCursorPos(as_glfw(handle_), pos.x, pos.y);
    last_cursor_pos_ = pos;
}

void window::set_cursor_pos(float64 x, float64 y) const {
    set_cursor_pos({x, y});
}

void window::set_cursor_mode(cursor_modes mode) const {
    glfwSetInputMode(as_glfw(handle_), GLFW_CURSOR, static_cast<int32>(mode));
}

void window::set_input_mode(input_modes mode, bool value) const {
    glfwSetInputMode(as_glfw(handle_), static_cast<int32>(mode), value ? GLFW_TRUE : GLFW_FALSE);
}

void window::set_title(std::string_view title) const {
    glfwSetWindowTitle(as_glfw(handle_), std::string(title).c_str());
}

void window::set_size(vec2i size) const {
    glfwSetWindowSize(as_glfw(handle_), size.x, size.y);
    size_ = size;
}

void window::set_size(int32 width, int32 height) const {
    set_size({width, height});
}

void window::set_position(vec2i pos) const {
    glfwSetWindowPos(as_glfw(handle_), pos.x, pos.y);
}

void window::set_position(int32 x, int32 y) const {
    set_position({x, y});
}

void window::maximize() const {
    glfwMaximizeWindow(as_glfw(handle_));
}

void window::minimize() const {
    glfwIconifyWindow(as_glfw(handle_));
}

void window::restore() const {
    glfwRestoreWindow(as_glfw(handle_));
}

}  // namespace vw::plat
