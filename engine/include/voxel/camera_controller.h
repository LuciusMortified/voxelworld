#pragma once
#include <memory>

#include <voxel/camera.h>
#include <voxel/events.h>
#include <voxel/input.h>

namespace voxel {
    class window;

    // Базовый абстрактный класс для управления камерой
    class camera_controller {
    public:
        virtual ~camera_controller() = default;
        
        virtual void initialize(std::shared_ptr<window> window, std::shared_ptr<camera> camera) = 0;
        virtual void update(float delta_time) = 0;
        
        virtual void set_camera(std::shared_ptr<camera> camera) = 0;
        virtual std::shared_ptr<camera> get_camera() const = 0;
        
        virtual void toggle_cursor_mode() = 0;
    };

    // Контроллер камеры в стиле FPS (First Person Shooter)
    class fps_camera_controller : public camera_controller {
    public:
        fps_camera_controller(
            float mouse_sensitivity = 0.1f,
            float camera_speed = 5.0f
        );
        ~fps_camera_controller() override;

        // Реализация базовых методов
        void initialize(std::shared_ptr<window> window, std::shared_ptr<camera> camera) override;
        void update(float delta_time) override;
        
        // Настройка контроллера
        void set_enabled(bool enabled) { enabled_ = enabled; }
        bool is_enabled() const { return enabled_; }
        
        // Настройка параметров
        void set_mouse_sensitivity(float sensitivity) { mouse_sensitivity_ = sensitivity; }
        float get_mouse_sensitivity() const { return mouse_sensitivity_; }
        
        void set_camera_speed(float speed) { camera_speed_ = speed; }
        float get_camera_speed() const { return camera_speed_; }
        
        bool is_mouse_captured() const { return mouse_captured_; }

        void set_camera(std::shared_ptr<camera> camera) override;
        std::shared_ptr<camera> get_camera() const override;
        void toggle_cursor_mode() override;

    private:
        void update_camera_movement(float delta_time);
        void update_camera_rotation();
        void handle_key_pressed(input::key key);
        void handle_mouse_moved(double x, double y);

        // Параметры управления
        float mouse_sensitivity_;
        float camera_speed_;
        bool mouse_captured_;
        bool enabled_;
        
        // Состояние мыши для вычисления дельты
        double last_mouse_x_;
        double last_mouse_y_;
        bool mouse_initialized_;
        
        // Указатели на объекты
        std::shared_ptr<camera> camera_;
        std::shared_ptr<window> window_;
        
        // Подписки на события
        events::sub_id key_press_subscription_;
        events::sub_id mouse_move_subscription_;
    };

    // Фабрика для создания контроллеров камеры
    class camera_controller_factory {
    public:
        static std::unique_ptr<camera_controller> create_fps_controller(
            float mouse_sensitivity = 0.1f,
            float camera_speed = 5.0f
        );
    };
} 