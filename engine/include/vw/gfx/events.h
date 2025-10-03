#pragma once

#include <map>
#include <functional>
#include <type_traits>

#include <vw/gfx/input.h>

namespace vw::gfx::events {
    struct event {
        virtual ~event() = default;
        bool handled = false;
    };

    struct key_press : public event {
        input::key key;
        int scancode;
        input::mod mods;
        
        key_press(input::key key, int scancode, input::mod mods)
            : key(key), scancode(scancode), mods(mods) {}

        bool with(input::mod mod) const {
            return (static_cast<int>(mods) & static_cast<int>(mod)) == static_cast<int>(mod);
        }
    };

    struct key_release : public event {
        input::key key;
        int scancode;
        input::mod mods;
        
        key_release(input::key key, int scancode, input::mod mods)
            : key(key), scancode(scancode), mods(mods) {}
    };

    struct key_repeat : public event {
        input::key key;
        int scancode;
        input::mod mods;
        
        key_repeat(input::key key, int scancode, input::mod mods)
            : key(key), scancode(scancode), mods(mods) {}
    };

    struct mouse_move : public event {
        double x;
        double y;
        
        mouse_move(double x, double y) : x(x), y(y) {}
    };

    struct mouse_press : public event {
        input::mouse_button button;
        input::mod mods;
        
        mouse_press(input::mouse_button button, input::mod mods)
            : button(button), mods(mods) {}
    };

    struct mouse_release : public event {
        input::mouse_button button;
        input::mod mods;
        
        mouse_release(input::mouse_button button, input::mod mods)
            : button(button), mods(mods) {}
    };

    struct mouse_scroll : public event {
        double offset_x;
        double offset_y;
        
        mouse_scroll(double offset_x, double offset_y)
            : offset_x(offset_x), offset_y(offset_y) {}
    };

    // События окна
    struct window_resize : public event {
        int width;
        int height;
        
        window_resize(int width, int height) : width(width), height(height) {}
    };

    struct window_focus : public event {
        bool focused;
        
        window_focus(bool focused) : focused(focused) {}
    };

    struct window_close : public event {};

    template<typename T>
    concept event_type = std::derived_from<T, event>;

    template<typename F, typename E>
    concept event_callback = event_type<E> && 
        std::invocable<F, E&> && 
        std::same_as<std::invoke_result_t<F, E&>, bool>;

    using sub_id = std::size_t;

    class event_dispatcher {
    public:
        template<event_type E, event_callback<E> F>
        sub_id subscribe(F&& callback) {
            auto& callbacks = get_callbacks<E>();
            sub_id id = next_id_++;
            callbacks[id] = std::forward<F>(callback);
            return id;
        }

        template<event_type E, event_callback<E> F>
        sub_id on(F&& callback) {
            return subscribe<E>(std::forward<F>(callback));
        }

        template<event_type E>
        bool dispatch(E& event) {
            auto& callbacks = get_callbacks<E>();
            
            for (auto& [id, callback] : callbacks) {
                if (callback(event)) {
                    event.handled = true;
                    return true;
                }
            }
            
            return false;
        }

        template<event_type E>
        void unsubscribe(sub_id id) {
            auto& callbacks = get_callbacks<E>();
            callbacks.erase(id);
        }

    private:
        sub_id next_id_ = 1;

        template<event_type E>
        static auto& get_callbacks() {
            static std::map<sub_id, std::function<bool(E&)>> callbacks;
            return callbacks;
        }
    };

} // namespace vw::gfx::events