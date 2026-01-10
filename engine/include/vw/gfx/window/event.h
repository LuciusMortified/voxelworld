#pragma once

#ifndef VW_GFX_EVENT_H
#define VW_GFX_EVENT_H

#include <functional>
#include <map>
#include <type_traits>

#include "vw/gfx/window/input.h"

namespace vw::gfx {
struct event {
    virtual ~event() = default;
    bool handled     = false;
};

struct key_press_event final : event {
    keyboard::keys key;
    int scancode;
    keyboard::mods mods;

    key_press_event(
        keyboard::keys key, int scancode, keyboard::mods mods
    )
        : key(key), scancode(scancode), mods(mods) {}

    [[nodiscard]]
    bool with(
        keyboard::mods mod
    ) const {
        return (static_cast<int>(mods) & static_cast<int>(mod)) == static_cast<int>(mod);
    }
};

struct key_release_event final : event {
    keyboard::keys key;
    int scancode;
    keyboard::mods mods;

    key_release_event(
        keyboard::keys key, int scancode, keyboard::mods mods
    )
        : key(key), scancode(scancode), mods(mods) {}
};

struct key_repeat_event final : event {
    keyboard::keys key;
    int scancode;
    keyboard::mods mods;

    key_repeat_event(
        keyboard::keys key, int scancode, keyboard::mods mods
    )
        : key(key), scancode(scancode), mods(mods) {}
};

struct mouse_move_event final : event {
    double x;
    double y;

    mouse_move_event(
        double x, double y
    )
        : x(x), y(y) {}
};

struct mouse_press_event final : event {
    mouse::button button;
    keyboard::mods mods;

    mouse_press_event(
        mouse::button button, keyboard::mods mods
    )
        : button(button), mods(mods) {}
};

struct mouse_release_event final : event {
    mouse::button button;
    keyboard::mods mods;

    mouse_release_event(
        mouse::button button, keyboard::mods mods
    )
        : button(button), mods(mods) {}
};

struct mouse_scroll_event final : event {
    double offset_x;
    double offset_y;

    mouse_scroll_event(
        double offset_x, double offset_y
    )
        : offset_x(offset_x), offset_y(offset_y) {}
};

struct window_resize_event final : event {
    int width;
    int height;

    window_resize_event(
        int width, int height
    )
        : width(width), height(height) {}
};

struct window_focus_event final : event {
    bool focused;

    explicit window_focus_event(
        bool focused
    )
        : focused(focused) {}
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

class event_dispatcher {
public:
    template <event_type E, event_callback_type<E> F>
    event_sub<E> sub(
        F&& callback
    ) {
        auto& callbacks     = get_callbacks<E>();
        event_sub<E> id     = {.value = next_id_++};
        callbacks[id.value] = std::forward<F>(callback);
        return id;
    }

    template <event_type E>
    void unsub(
        event_sub<E> id
    ) {
        auto& callbacks = get_callbacks<E>();
        callbacks.erase(id);
    }

    template <event_type E>
    void dispatch(
        E& event
    ) {
        for (auto& [id, callback] : get_callbacks<E>()) {
            if (callback(event)) {
                event.handled = true;
            }
        }
    }

private:
    std::size_t next_id_ = 1;

    template <event_type E>
    static auto& get_callbacks() {
        static std::map<std::size_t, std::function<bool(E&)>> callbacks;
        return callbacks;
    }
};

}  // namespace vw::gfx

#endif  // VW_GFX_EVENT_H
