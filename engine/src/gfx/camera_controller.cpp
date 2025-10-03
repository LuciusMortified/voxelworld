#include <vw/gfx/camera_controller.h>
#include <vw/gfx/input.h>
#include <vw/gfx/window.h>

namespace vw::gfx {

// FPS Camera Controller Implementation
fps_camera_controller::fps_camera_controller(
    float mouse_sensitivity,
    float camera_speed
)
    : mouse_sensitivity_(mouse_sensitivity),
      camera_speed_(camera_speed),
      mouse_captured_(false),
      enabled_(true),
      last_mouse_x_(0.0),
      last_mouse_y_(0.0),
      mouse_initialized_(false),
      key_press_subscription_(0),
      mouse_move_subscription_(0) {}

void fps_camera_controller::setup(window& window, camera& camera) {
    window_ = &window;
    camera_ = &camera;

    // Инициализация позиции мыши
    window_->get_cursor_pos(&last_mouse_x_, &last_mouse_y_);
    mouse_initialized_ = true;

    // Получаем event_dispatcher из window
    auto& event_dispatcher = window_->get_event_dispatcher();

    // Подписка на события клавиатуры
    key_press_subscription_ = window_->on<events::key_press>(
        [this](events::key_press& event) -> bool {
            handle_key_pressed(event.key);
            return false;
        }
    );

    // Подписка на события движения мыши
    mouse_move_subscription_ = window_->on<events::mouse_move>(
        [this](events::mouse_move& event) -> bool {
            if (mouse_captured_) {
                handle_mouse_moved(event.x, event.y);
            }
            return false;
        }
    );
}

void fps_camera_controller::update(float delta_time) {
    if (!enabled_ || !camera_ || !window_)
        return;

    update_camera_movement(delta_time);
}

void fps_camera_controller::handle_key_pressed(input::key key) {
    switch (key) {
        case input::key::TAB:
            toggle_mouse_captured();
            break;
        default:
            break;
    }
}

void fps_camera_controller::handle_mouse_moved(double x, double y) {
    if (!camera_) {
        return;
    }

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

    if (delta_x != 0 || delta_y != 0) {
        float yaw_delta = static_cast<float>(delta_x) * mouse_sensitivity_;
        float pitch_delta = static_cast<float>(delta_y) * mouse_sensitivity_;

        camera_->rotate(
            -pitch_delta,
            yaw_delta
        );  // Инвертируем pitch для Vulkan
    }
}

void fps_camera_controller::update_camera_movement(float delta_time) const {
    const float move_speed = camera_speed_ * delta_time;

    if (window_->is_key_pressed(input::key::W)) {
        camera_->move_forward(move_speed);
    }
    if (window_->is_key_pressed(input::key::S)) {
        camera_->move_forward(-move_speed);
    }

    if (window_->is_key_pressed(input::key::A)) {
        camera_->move_right(-move_speed);
    }
    if (window_->is_key_pressed(input::key::D)) {
        camera_->move_right(move_speed);
    }

    if (window_->is_key_pressed(input::key::SPACE)) {
        const vec3f world_up(0.0f, 1.0f, 0.0f);
        camera_->set_position(camera_->get_position() + world_up * move_speed);
    }
    if (window_->is_key_pressed(input::key::LEFT_SHIFT)) {
        const vec3f world_up(0.0f, 1.0f, 0.0f);
        camera_->set_position(camera_->get_position() - world_up * move_speed);
    }
}

void fps_camera_controller::set_mouse_captured(bool captured) {
    mouse_captured_ = captured;

    window_->set_cursor_mode(
        mouse_captured_ ? input::cursor_mode::DISABLED :
                          input::cursor_mode::NORMAL
    );

    mouse_initialized_ = false;
}

void fps_camera_controller::toggle_mouse_captured() {
    set_mouse_captured(!mouse_captured_);
}

}  // namespace vw::gfx