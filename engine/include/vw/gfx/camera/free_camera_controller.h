#pragma once

#ifndef VW_GFX_CAMERA_FREE_CAMERA_CONTROLLER_H
#define VW_GFX_CAMERA_FREE_CAMERA_CONTROLLER_H

#include "vw/gfx/camera/camera.h"
#include "vw/gfx/window/event.h"
#include "vw/gfx/window/input.h"

namespace vw::gfx {
class window;

class free_camera_controller {
public:
    explicit free_camera_controller(float mouse_sensitivity = 0.1f, float camera_speed = 5.0f);
    virtual ~free_camera_controller() = default;

    void setup(window& window, camera& camera);
    void update(float delta_time) const;

    void set_mouse_sensitivity(float sensitivity);
    [[nodiscard]] auto get_mouse_sensitivity() const -> float;

    void set_camera_speed(float speed);
    [[nodiscard]] auto get_camera_speed() const -> float;

    [[nodiscard]] auto is_mouse_captured() const -> bool;
    void set_mouse_captured(bool captured);
    void toggle_mouse_captured();

    [[nodiscard]] auto keyboard_control_enabled() const -> bool;
    void set_keyboard_control_enabled(bool enabled);
    void toggle_keyboard_control_enabled();

private:
    void update_camera_movement_(float delta_time) const;
    void handle_mouse_moved_(double x, double y);

    float mouse_sensitivity_;
    float camera_speed_;

    bool mouse_captured_{false};
    bool keyboard_control_enabled_{false};

    double last_mouse_x_{0.0};
    double last_mouse_y_{0.0};
    bool mouse_initialized_{false};

    window* window_ = nullptr;
    camera* camera_ = nullptr;

    event_sub<key_press_event> key_press_sub_;
    event_sub<mouse_move_event> mouse_move_sub_;
};
}  // namespace vw::gfx

#include "vw/gfx/camera/free_camera_controller.inl.h"

#endif  // VW_GFX_CAMERA_FREE_CAMERA_CONTROLLER_H
