module vw.gfx;

import std;
import vw.core;
import vw.ecs;
import vw.world;
import vw.platform;

namespace vw::gfx {

free_camera_controller::free_camera_controller(
    float mouse_sensitivity, float camera_speed
)
    : mouse_sensitivity_(mouse_sensitivity)
    , camera_speed_(camera_speed)
    , key_press_sub_(0)
    , mouse_move_sub_(0) {}

void free_camera_controller::setup(
    window& window, camera& camera
) {
    window_ = &window;
    camera_ = &camera;

    const auto cursor_pos = window_->get_cursor_pos();
    last_mouse_x_         = cursor_pos.x;
    last_mouse_y_         = cursor_pos.y;

    mouse_initialized_ = true;

    mouse_move_sub_ = window_->sub<mouse_move_event>([this](const mouse_move_event& event) -> bool {
        if (mouse_captured_) {
            handle_mouse_moved_(event.x, event.y);
        }
        return false;
    });
}

void free_camera_controller::update(
    float delta_time
) const {
    if (!camera_ || !window_)
        return;

    update_camera_movement_(delta_time);
}

void free_camera_controller::set_mouse_sensitivity(
    float sensitivity
) {
    mouse_sensitivity_ = sensitivity;
}

float free_camera_controller::get_mouse_sensitivity() const {
    return mouse_sensitivity_;
}

void free_camera_controller::set_camera_speed(
    float speed
) {
    camera_speed_ = speed;
}

float free_camera_controller::get_camera_speed() const {
    return camera_speed_;
}

bool free_camera_controller::is_mouse_captured() const {
    return mouse_captured_;
}

void free_camera_controller::handle_mouse_moved_(
    double x, double y
) {
    if (!camera_) {
        return;
    }

    if (!mouse_initialized_) {
        last_mouse_x_      = x;
        last_mouse_y_      = y;
        mouse_initialized_ = true;
        return;
    }

    const double delta_x = x - last_mouse_x_;
    const double delta_y = y - last_mouse_y_;

    last_mouse_x_ = x;
    last_mouse_y_ = y;

    if (delta_x != 0 || delta_y != 0) {
        const float yaw_delta   = static_cast<float>(delta_x) * mouse_sensitivity_;
        const float pitch_delta = -static_cast<float>(delta_y) * mouse_sensitivity_;

        camera_->rotate(pitch_delta, yaw_delta);
    }
}

void free_camera_controller::update_camera_movement_(
    float delta_time
) const {
    if (!keyboard_control_enabled_) {
        return;
    }

    const float move_speed = camera_speed_ * delta_time;

    if (window_->is_key_pressed(keyboard::keys::W)) {
        camera_->move_forward(move_speed);
    }
    if (window_->is_key_pressed(keyboard::keys::S)) {
        camera_->move_forward(-move_speed);
    }

    if (window_->is_key_pressed(keyboard::keys::A)) {
        camera_->move_right(-move_speed);
    }
    if (window_->is_key_pressed(keyboard::keys::D)) {
        camera_->move_right(move_speed);
    }

    if (window_->is_key_pressed(keyboard::keys::SPACE)) {
        constexpr vec3f world_up(0.0f, 1.0f, 0.0f);
        camera_->set_position(camera_->get_position() + world_up * move_speed);
    }
    if (window_->is_key_pressed(keyboard::keys::LEFT_SHIFT)) {
        constexpr vec3f world_up(0.0f, 1.0f, 0.0f);
        camera_->set_position(camera_->get_position() - world_up * move_speed);
    }
}

void free_camera_controller::set_mouse_captured(
    bool captured
) {
    mouse_captured_ = captured;

    window_->set_cursor_mode(mouse_captured_ ? cursor_modes::DISABLED : cursor_modes::NORMAL);
    window_->set_input_mode(input_modes::RAW_MOUSE_MOTION, mouse_captured_);

    mouse_initialized_ = false;
}

void free_camera_controller::toggle_mouse_captured() {
    set_mouse_captured(!mouse_captured_);
}

bool free_camera_controller::keyboard_control_enabled() const {
    return keyboard_control_enabled_;
}

void free_camera_controller::set_keyboard_control_enabled(
    bool enabled
) {
    keyboard_control_enabled_ = enabled;
}

void free_camera_controller::toggle_keyboard_control_enabled() {
    keyboard_control_enabled_ = !keyboard_control_enabled_;
}

}  // namespace vw::gfx
