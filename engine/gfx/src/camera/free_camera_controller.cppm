export module vw.gfx:camera.free_controller;

import std;

import vw.core;
import vw.ecs;
import vw.world;
import vw.platform;
import :camera;

namespace vw::gfx {
using namespace ::vw::ecs;
using namespace ::vw::plat;
}

export namespace vw::gfx {

class free_camera_controller {
public:
    explicit free_camera_controller(float mouse_sensitivity = 0.1f, float camera_speed = 5.0f);
    virtual ~free_camera_controller() = default;

    auto setup(window& window, camera& camera) -> void;
    auto update(float delta_time) const -> void;

    auto set_mouse_sensitivity(float sensitivity) -> void;
    [[nodiscard]] auto get_mouse_sensitivity() const -> float;

    auto set_camera_speed(float speed) -> void;
    [[nodiscard]] auto get_camera_speed() const -> float;

    [[nodiscard]] auto is_mouse_captured() const -> bool;
    auto set_mouse_captured(bool captured) -> void;
    auto toggle_mouse_captured() -> void;

    [[nodiscard]] auto keyboard_control_enabled() const -> bool;
    auto set_keyboard_control_enabled(bool enabled) -> void;
    auto toggle_keyboard_control_enabled() -> void;

private:
    auto update_camera_movement_(float delta_time) const -> void;
    auto handle_mouse_moved_(double x, double y) -> void;

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
