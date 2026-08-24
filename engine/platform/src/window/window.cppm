export module vw.platform:window;

import std;

import vw.core;
import :event;
import :input;

namespace vw::plat::detail {
struct window_callbacks;
}  // namespace vw::plat::detail

export namespace vw::plat {

// Единственное окно, о котором знает движок. Ничто из оконного бэкенда не
// доходит до этого интерфейса: сюрфейс едет непрозрачным дескриптором, поэтому
// gfx остаётся на типах vk::, а platform их не видит вовсе.
class window final {
public:
    window(int32 width, int32 height, std::string_view title);
    ~window();

    window(const window&)                      = delete;
    auto operator=(const window&) -> window&   = delete;
    window(window&&)                           = delete;
    auto operator=(window&&) -> window&        = delete;

    [[nodiscard]] auto should_close() const -> bool;

    void poll_events();

    [[nodiscard]] auto framebuffer_size() const -> vec2i;

    // Принимает VkInstance, возвращает VkSurfaceKHR — оба сырыми дескрипторами,
    // чтобы заголовки Vulkan остались на стороне вызывающего.
    [[nodiscard]] auto create_surface(uint64 instance) const -> uint64;

    [[nodiscard]] static auto required_extensions() -> std::vector<const char*>;

    [[nodiscard]] auto get_size() const -> vec2i {
        return size_;
    }

    [[nodiscard]] auto get_width() const -> int32 {
        return size_.x;
    }

    [[nodiscard]] auto get_height() const -> int32 {
        return size_.y;
    }

    [[nodiscard]] auto is_key_pressed(keyboard::keys key) const -> bool;
    [[nodiscard]] auto is_mouse_button_pressed(mouse::buttons button) const -> bool;

    [[nodiscard]] auto get_cursor_pos() const -> vec2d;

    void set_cursor_pos(vec2d pos) const;
    void set_cursor_pos(float64 x, float64 y) const;

    void set_cursor_mode(cursor_modes mode) const;
    void set_input_mode(input_modes mode, bool value) const;

    void set_title(std::string_view title) const;

    void set_size(vec2i size) const;
    void set_size(int32 width, int32 height) const;

    void set_position(vec2i pos) const;
    void set_position(int32 x, int32 y) const;

    void maximize() const;
    void minimize() const;
    void restore() const;

    // GLFWwindow* для бэкенда imgui, которому нужен нативный дескриптор.
    [[nodiscard]] auto native_handle() const -> void* {
        return handle_;
    }

    template <event_type E, event_callback_type<E> F>
    auto sub(F&& callback) -> event_sub<E> {
        return event_dispatcher_.sub<E>(std::forward<F>(callback));
    }

    template <event_type E>
    void unsub(event_sub<E> sub) {
        return event_dispatcher_.unsub(sub);
    }

private:
    // Оконный бэкенд передаёт своим колбэкам нативный дескриптор, поэтому они
    // живут в имплементационном юните и добираются до диспетчера через это.
    friend struct detail::window_callbacks;

    void on_key_(int32 key, int32 scancode, int32 mods, bool pressed, bool repeat);
    void on_mouse_button_(int32 button, int32 mods, bool pressed);
    void on_mouse_move_(float64 x, float64 y);
    void on_mouse_scroll_(float64 offset_x, float64 offset_y);
    void on_resize_(int32 width, int32 height);
    void on_focus_(bool focused);
    void on_close_();

    void* handle_ = nullptr;

    mutable vec2i size_;
    mutable vec2d last_cursor_pos_{0.0, 0.0};

    event_dispatcher event_dispatcher_;
};

}  // namespace vw::plat
