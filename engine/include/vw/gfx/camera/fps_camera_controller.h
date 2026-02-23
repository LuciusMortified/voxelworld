#pragma once

#ifndef VW_GFX_CAMERA_CAMERA_CONTROLLER_H
#define VW_GFX_CAMERA_CAMERA_CONTROLLER_H

#include "vw/gfx/camera/camera.h"
#include "vw/gfx/window/event.h"
#include "vw/gfx/window/input.h"

namespace vw::gfx {
class window;

class fps_camera_controller {
public:
    explicit fps_camera_controller(float mouse_sensitivity = 0.1f, float camera_speed = 5.0f);
    virtual ~fps_camera_controller() = default;

    void setup(window& window, camera& camera);
    void update(float delta_time) const;

    void set_mouse_sensitivity(float sensitivity);
    [[nodiscard]] float get_mouse_sensitivity() const;

    void set_camera_speed(float speed);
    [[nodiscard]] float get_camera_speed() const;

    [[nodiscard]] bool is_mouse_captured() const;
    void set_mouse_captured(bool captured);
    void toggle_mouse_captured();

    [[nodiscard]] bool keyboard_control_enabled() const;
    void set_keyboard_control_enabled(bool enabled);
    void toggle_keyboard_control_enabled();

private:
    void update_camera_movement_(float delta_time) const;
    void handle_mouse_moved_(double x, double y);

    float mouse_sensitivity_;
    float camera_speed_;

    bool mouse_captured_;
    bool keyboard_control_enabled_;

    double last_mouse_x_;
    double last_mouse_y_;
    bool mouse_initialized_;

    window* window_ = nullptr;
    camera* camera_ = nullptr;

    event_sub<key_press_event> key_press_sub_;
    event_sub<mouse_move_event> mouse_move_sub_;
};
}  // namespace vw::gfx

#include "vw/gfx/camera/fps_camera_controller.inl.h"

#endif  // VW_GFX_CAMERA_CAMERA_CONTROLLER_H
