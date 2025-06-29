#pragma once
#include <functional>
#include <vector>
#include <unordered_map>

#include <voxel/input.h>

namespace voxel {
namespace events {

    // Базовый класс для всех событий
    struct event {
        virtual ~event() = default;
        bool handled = false;
    };

    // События клавиатуры
    struct key_press : public event {
        input::key key;
        int scancode;
        input::mod mods;
        
        key_press(input::key key, int scancode, input::mod mods)
            : key(key), scancode(scancode), mods(mods) {}
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

    // События мыши
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

    // Концепты для ограничения типов событий
    template<typename T>
    concept event_type = std::derived_from<T, event>;

    // Концепт для callback функций
    template<typename F, typename E>
    concept event_callback = event_type<E> && 
        std::invocable<F, E&> && 
        std::same_as<std::invoke_result_t<F, E&>, bool>;

    using sub_id = std::size_t;

    // Обработчик событий с типобезопасными перегрузками
    class event_dispatcher {
    public:
        // Подписка на события с шаблонными параметрами
        template<event_type E, event_callback<E> F>
        sub_id subscribe(F&& callback) {
            auto& callbacks = get_callbacks<E>();
            sub_id id = next_id_++;
            callbacks[id] = std::forward<F>(callback);
            return id;
        }

        // Универсальный метод для подписки на события
        template<event_type E, event_callback<E> F>
        sub_id on(F&& callback) {
            return subscribe<E>(std::forward<F>(callback));
        }

        // Диспетчеризация событий
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

        // Отписка от событий
        template<event_type E>
        void unsubscribe(sub_id id) {
            auto& callbacks = get_callbacks<E>();
            callbacks.erase(id);
        }

    private:
        std::size_t next_id_ = 1;

        // Вспомогательный метод для получения нужного контейнера
        template<event_type E>
        static auto& get_callbacks() {
            static std::unordered_map<sub_id, std::function<bool(E&)>> callbacks;
            return callbacks;
        }
    };

} // namespace events
} // namespace voxel 