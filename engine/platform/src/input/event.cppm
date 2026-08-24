export module vw.platform:event;

import std;

import vw.core;
import :input;

export namespace vw::plat {

struct event {
    virtual ~event() = default;
    bool handled     = false;
};

struct key_press_event final : event {
    keyboard::keys key;
    int32 scancode;
    keyboard::mods mods;

    key_press_event(keyboard::keys key, int32 scancode, keyboard::mods mods)
        : key(key), scancode(scancode), mods(mods) {}

    [[nodiscard]] auto with(keyboard::mods mod) const -> bool {
        return (static_cast<int32>(mods) & static_cast<int32>(mod)) == static_cast<int32>(mod);
    }
};

struct key_release_event final : event {
    keyboard::keys key;
    int32 scancode;
    keyboard::mods mods;

    key_release_event(keyboard::keys key, int32 scancode, keyboard::mods mods)
        : key(key), scancode(scancode), mods(mods) {}
};

struct key_repeat_event final : event {
    keyboard::keys key;
    int32 scancode;
    keyboard::mods mods;

    key_repeat_event(keyboard::keys key, int32 scancode, keyboard::mods mods)
        : key(key), scancode(scancode), mods(mods) {}
};

struct mouse_move_event final : event {
    float64 x;
    float64 y;

    mouse_move_event(float64 x, float64 y) : x(x), y(y) {}
};

struct mouse_press_event final : event {
    mouse::buttons button;
    keyboard::mods mods;

    mouse_press_event(mouse::buttons button, keyboard::mods mods) : button(button), mods(mods) {}
};

struct mouse_release_event final : event {
    mouse::buttons button;
    keyboard::mods mods;

    mouse_release_event(mouse::buttons button, keyboard::mods mods) : button(button), mods(mods) {}
};

struct mouse_scroll_event final : event {
    float64 offset_x;
    float64 offset_y;

    mouse_scroll_event(float64 offset_x, float64 offset_y)
        : offset_x(offset_x), offset_y(offset_y) {}
};

struct window_resize_event final : event {
    int32 width;
    int32 height;

    window_resize_event(int32 width, int32 height) : width(width), height(height) {}
};

struct window_focus_event final : event {
    bool focused;

    explicit window_focus_event(bool focused) : focused(focused) {}
};

struct window_close_event final : event {};

template <typename T>
concept event_type = std::derived_from<T, event>;

template <typename F, typename E>
concept event_callback_type =
    event_type<E> && std::invocable<F, E&> && std::same_as<std::invoke_result_t<F, E&>, bool>;

template <event_type>
struct event_sub {
    std::size_t value = 0;
};

// Колбэки хранятся по типу события, поэтому диспетчер никогда не смотрит на
// события, на которые никто не подписан.
class event_dispatcher {
public:
    template <event_type E, event_callback_type<E> F>
    auto sub(F&& callback) -> event_sub<E> {
        auto& callbacks     = get_callbacks<E>();
        event_sub<E> id     = {.value = next_id_++};
        callbacks[id.value] = std::forward<F>(callback);
        return id;
    }

    template <event_type E>
    void unsub(event_sub<E> id) {
        get_callbacks<E>().erase(id.value);
    }

    template <event_type E>
    void dispatch(E& event) {
        for (auto& [id, callback] : get_callbacks<E>()) {
            if (callback(event)) {
                event.handled = true;
            }
        }
    }

private:
    template <event_type E>
    static auto get_callbacks() -> auto& {
        static std::map<std::size_t, std::function<bool(E&)>> callbacks;
        return callbacks;
    }

    std::size_t next_id_ = 1;
};

}  // namespace vw::plat
