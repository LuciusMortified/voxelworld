#include <voxel/camera_controller.h>
#include <voxel/window.h>
#include <voxel/input.h>

namespace voxel {

    // FPS Camera Controller Implementation
    fps_camera_controller::fps_camera_controller(
        float mouse_sensitivity,
        float camera_speed
    ) : mouse_sensitivity_(mouse_sensitivity),
        camera_speed_(camera_speed),
        mouse_captured_(false),
        enabled_(true),
        last_mouse_x_(0.0),
        last_mouse_y_(0.0),
        mouse_initialized_(false),
        camera_(nullptr),
        window_(nullptr),
        key_press_subscription_(0),
        mouse_move_subscription_(0) {
    }

    fps_camera_controller::~fps_camera_controller() {
        // Отписка от событий будет происходить в деструкторе
        // Но для этого нужен доступ к event_dispatcher, который может быть недоступен
        // Поэтому просто обнуляем подписки
        key_press_subscription_ = 0;
        mouse_move_subscription_ = 0;
    }

    void fps_camera_controller::initialize(std::shared_ptr<window> window, std::shared_ptr<camera> camera) {
        window_ = window;
        camera_ = camera;
        
        // Инициализация позиции мыши
        window_->get_cursor_pos(&last_mouse_x_, &last_mouse_y_);
        mouse_initialized_ = true;
        
        // Получаем event_dispatcher из window
        auto& event_dispatcher = window_->get_event_dispatcher();
        
        // Подписка на события клавиатуры
        key_press_subscription_ = event_dispatcher.on<events::key_press>(
            [this](events::key_press& event) -> bool {
                handle_key_pressed(event.key);
                return false; // Не обрабатываем событие полностью
            }
        );
        
        // Подписка на события движения мыши
        mouse_move_subscription_ = event_dispatcher.on<events::mouse_move>(
            [this](events::mouse_move& event) -> bool {
                if (mouse_captured_) {
                    handle_mouse_moved(event.x, event.y);
                }
                return false; // Не обрабатываем событие полностью
            }
        );
    }

    void fps_camera_controller::update(float delta_time) {
        if (!enabled_ || !camera_ || !window_) return;
        
        // Обновляем движение камеры
        update_camera_movement(delta_time);
        
        // Обновляем поворот камеры
        update_camera_rotation();
    }

    void fps_camera_controller::handle_key_pressed(input::key key) {
        switch (key) {
            case input::key::TAB:
                toggle_cursor_mode();
                break;
            default:
                break;
        }
    }

    void fps_camera_controller::handle_mouse_moved(double x, double y) {
        // Инициализация позиции мыши при первом захвате
        if (!mouse_initialized_) {
            last_mouse_x_ = x;
            last_mouse_y_ = y;
            mouse_initialized_ = true;
            return;
        }
        
        // Вычисляем дельту движения мыши
        double delta_x = x - last_mouse_x_;
        double delta_y = y - last_mouse_y_;
        
        last_mouse_x_ = x;
        last_mouse_y_ = y;
        
        // Применяем поворот камеры
        if (delta_x != 0 || delta_y != 0) {
            float yaw_delta = static_cast<float>(delta_x) * mouse_sensitivity_;
            float pitch_delta = static_cast<float>(delta_y) * mouse_sensitivity_;
            
            camera_->rotate(-pitch_delta, yaw_delta);
        }
    }

    void fps_camera_controller::update_camera_movement(float delta_time) {
        float move_speed = camera_speed_ * delta_time;
        
        // Движение вперед/назад (W/S)
        if (window_->is_key_pressed(input::key::W)) {
            camera_->move_forward(move_speed);
        }
        if (window_->is_key_pressed(input::key::S)) {
            camera_->move_forward(-move_speed);
        }
        
        // Движение влево/вправо (A/D)
        if (window_->is_key_pressed(input::key::A)) {
            camera_->move_right(-move_speed);
        }
        if (window_->is_key_pressed(input::key::D)) {
            camera_->move_right(move_speed);
        }
        
        // Движение вверх/вниз (Space/Shift)
        if (window_->is_key_pressed(input::key::SPACE)) {
            camera_->move_up(move_speed);
        }
        if (window_->is_key_pressed(input::key::LEFT_SHIFT)) {
            camera_->move_up(-move_speed);
        }
    }

    void fps_camera_controller::update_camera_rotation() {
        if (!mouse_captured_) return;
        
        double mouse_x, mouse_y;
        window_->get_cursor_pos(&mouse_x, &mouse_y);
        
        // Инициализация позиции мыши при первом захвате
        if (!mouse_initialized_) {
            last_mouse_x_ = mouse_x;
            last_mouse_y_ = mouse_y;
            mouse_initialized_ = true;
            return;
        }
        
        // Вычисляем дельту движения мыши
        double delta_x = mouse_x - last_mouse_x_;
        double delta_y = mouse_y - last_mouse_y_;
        
        last_mouse_x_ = mouse_x;
        last_mouse_y_ = mouse_y;
        
        // Применяем поворот камеры
        if (delta_x != 0 || delta_y != 0) {
            float yaw_delta = static_cast<float>(delta_x) * mouse_sensitivity_;
            float pitch_delta = static_cast<float>(delta_y) * mouse_sensitivity_;
            
            camera_->rotate(-pitch_delta, yaw_delta);
        }
    }

    void fps_camera_controller::set_camera(std::shared_ptr<camera> camera) {
        camera_ = camera;
    }

    std::shared_ptr<camera> fps_camera_controller::get_camera() const {
        return camera_;
    }

    void fps_camera_controller::toggle_cursor_mode() {
        mouse_captured_ = !mouse_captured_;
        if (window_) {
            window_->set_cursor_mode(mouse_captured_ ? 
                input::cursor_mode::DISABLED : input::cursor_mode::NORMAL);
            
            // Сбрасываем состояние мыши при переключении
            mouse_initialized_ = false;
        }
    }

    // Factory Implementation
    std::unique_ptr<camera_controller> camera_controller_factory::create_fps_controller(
        float mouse_sensitivity,
        float camera_speed
    ) {
        return std::make_unique<fps_camera_controller>(mouse_sensitivity, camera_speed);
    }

} // namespace voxel 