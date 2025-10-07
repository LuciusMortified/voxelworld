#include "vw/gfx/camera/camera_controller.h"

#include "vw/gfx/window/window.h"

namespace vw::gfx {

fps_camera_controller::fps_camera_controller(float mouse_sensitivity, float camera_speed)
    : mouse_sensitivity_(mouse_sensitivity),
      camera_speed_(camera_speed),
      mouse_captured_(false),
      enabled_(true),
      last_mouse_x_(0.0),
      last_mouse_y_(0.0),
      mouse_initialized_(false),
      key_press_sub_(0),
      mouse_move_sub_(0) {}

void fps_camera_controller::setup(window& window, camera& camera) {
    window_ = &window;
    camera_ = &camera;

    const auto cursor_pos = window_->get_cursor_pos();
    last_mouse_x_         = cursor_pos.x;
    last_mouse_y_         = cursor_pos.y;

    mouse_initialized_ = true;

    key_press_sub_ = window_->sub<key_press_event>([this](const key_press_event& event) -> bool {
        handle_key_pressed(event.key);
        return false;
    });

    mouse_move_sub_ = window_->sub<mouse_move_event>([this](const mouse_move_event& event) -> bool {
        if (mouse_captured_) {
            handle_mouse_moved(event.x, event.y);
        }
        return false;
    });
}

void fps_camera_controller::update(float delta_time) {
    if (!enabled_ || !camera_ || !window_)
        return;

    update_camera_movement(delta_time);
}

void fps_camera_controller::handle_key_pressed(keyboard::key key) {
    switch (key) {
        case keyboard::key::TAB:
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
        last_mouse_x_      = x;
        last_mouse_y_      = y;
        mouse_initialized_ = true;
        return;
    }

    // Вычисляем дельту движения мыши
    double delta_x = x - last_mouse_x_;
    double delta_y = y - last_mouse_y_;

    last_mouse_x_ = x;
    last_mouse_y_ = y;

    if (delta_x != 0 || delta_y != 0) {
        float yaw_delta   = static_cast<float>(delta_x) * mouse_sensitivity_;
        float pitch_delta = static_cast<float>(delta_y) * mouse_sensitivity_;

        camera_->rotate(-pitch_delta,
                        yaw_delta);  // Инвертируем pitch для Vulkan
    }
}

void fps_camera_controller::update_camera_movement(float delta_time) const {
    const float move_speed = camera_speed_ * delta_time;

    if (window_->is_key_pressed(keyboard::key::W)) {
        camera_->move_forward(move_speed);
    }
    if (window_->is_key_pressed(keyboard::key::S)) {
        camera_->move_forward(-move_speed);
    }

    if (window_->is_key_pressed(keyboard::key::A)) {
        camera_->move_right(-move_speed);
    }
    if (window_->is_key_pressed(keyboard::key::D)) {
        camera_->move_right(move_speed);
    }

    if (window_->is_key_pressed(keyboard::key::SPACE)) {
        const vec3f world_up(0.0f, 1.0f, 0.0f);
        camera_->set_position(camera_->get_position() + world_up * move_speed);
    }
    if (window_->is_key_pressed(keyboard::key::LEFT_SHIFT)) {
        const vec3f world_up(0.0f, 1.0f, 0.0f);
        camera_->set_position(camera_->get_position() - world_up * move_speed);
    }
}

void fps_camera_controller::set_mouse_captured(bool captured) {
    mouse_captured_ = captured;

    window_->set_cursor_mode(mouse_captured_ ? cursor_mode::DISABLED : cursor_mode::NORMAL);

    mouse_initialized_ = false;
}

void fps_camera_controller::toggle_mouse_captured() {
    set_mouse_captured(!mouse_captured_);
}

}  // namespace vw::gfx