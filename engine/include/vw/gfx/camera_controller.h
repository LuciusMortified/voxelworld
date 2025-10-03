#pragma once
#include <memory>

#include "vw/gfx/camera.h"
#include "vw/gfx/events.h"
#include "vw/gfx/input.h"

namespace vw::gfx {

    class window;

    class fps_camera_controller {
    public:
        explicit fps_camera_controller(
            float mouse_sensitivity = 0.1f,
            float camera_speed = 5.0f
        );
        virtual ~fps_camera_controller() = default;

        void setup(window& window, camera& camera);

        void update(float delta_time);
        
        void set_mouse_sensitivity(float sensitivity) { mouse_sensitivity_ = sensitivity; }

        [[nodiscard]]
        float get_mouse_sensitivity() const { return mouse_sensitivity_; }
        
        void set_camera_speed(float speed) { camera_speed_ = speed; }

        [[nodiscard]]
        float get_camera_speed() const { return camera_speed_; }

        [[nodiscard]]
        bool is_mouse_captured() const { return mouse_captured_; }

        void set_mouse_captured(bool captured);

        void toggle_mouse_captured();

    private:
        void update_camera_movement(float delta_time) const;
        void handle_key_pressed(input::key key);
        void handle_mouse_moved(double x, double y);

        float mouse_sensitivity_;
        float camera_speed_;
        bool mouse_captured_;
        bool enabled_;
        
        double last_mouse_x_;
        double last_mouse_y_;
        bool mouse_initialized_;
        
        window* window_ = nullptr;
        camera* camera_ = nullptr;
        
        events::sub_id key_press_subscription_;
        events::sub_id mouse_move_subscription_;
    };
} 