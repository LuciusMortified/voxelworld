export module vw.gfx:camera;

import std;

import vw.core;
import vw.ecs;
import vw.world;
import vw.platform;

namespace vw::gfx {
using namespace ::vw::ecs;
using namespace ::vw::plat;
}

// ---- from vw/gfx/player/player_input_state.h
export namespace vw::gfx {

struct player_input_state {
    vec2f move_input{0.0f, 0.0f};
    vec2f look_delta{0.0f, 0.0f};
    float32 zoom_delta    = 0.0f;
    bool jump_requested   = false;
    bool attack_requested = false;
    bool sprint           = false;
};

}  // namespace vw::gfx

// ---- from vw/gfx/camera/camera.h
export namespace vw::gfx {


class camera {
public:
    explicit camera(
        float fov = 60.0f, float aspect = 16.0f / 9.0f, float near = 0.1f, float far = 50000.0f
    );

    auto set_position(const vec3f& position) -> void;
    auto set_rotation(float pitch, float yaw) -> void;
    auto set_aspect_ratio(float aspect) -> void;

    [[nodiscard]] auto get_near() const -> float;
    [[nodiscard]] auto get_far() const -> float;
    [[nodiscard]] auto get_fov() const -> float;
    [[nodiscard]] auto get_aspect_ratio() const -> float;

    [[nodiscard]] auto get_position() const -> vec3f;
    [[nodiscard]] auto get_pitch() const -> float;
    [[nodiscard]] auto get_yaw() const -> float;

    [[nodiscard]] auto get_view_matrix() const -> mat4f;
    [[nodiscard]] auto get_projection_matrix() const -> mat4f;
    [[nodiscard]] auto get_view_projection_matrix() const -> mat4f;

    [[nodiscard]] auto get_frustum() const -> const vw::spatial::frustum&;

    auto move_forward(float distance) -> void;
    auto move_right(float distance) -> void;
    auto move_up(float distance) -> void;
    auto rotate(float delta_pitch, float delta_yaw) -> void;

    auto get_forward() const -> vec3f;
    auto get_right() const -> vec3f;
    auto get_up() const -> vec3f;

    [[nodiscard]] auto screen_to_world_ray(
        const vec2d& mouse_pos,
        const vec2i& window_size
    ) const -> vw::spatial::ray;

private:
    auto update_vectors() const -> void;
    auto update_view_matrix() const -> void;
    auto update_projection_matrix() const -> void;
    auto update_frustum() const -> void;

    vec3f position_;
    float pitch_, yaw_;
    float fov_, aspect_, near_, far_;

    mutable vec3f forward_, right_, up_;
    mutable bool vectors_dirty_;

    mutable mat4f view_matrix_;
    mutable mat4f projection_matrix_;
    mutable bool view_matrix_dirty_;
    mutable bool projection_matrix_dirty_;
    
    mutable vw::spatial::frustum frustum_;
    mutable bool frustum_dirty_;
};
}  // namespace vw::gfx

// ---- from vw/gfx/camera/fps_camera_controller.h
export namespace vw::gfx {

class fps_camera_controller {
public:
    explicit fps_camera_controller(float mouse_sensitivity = 0.1f, float camera_speed = 5.0f);
    virtual ~fps_camera_controller() = default;

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

// ---- from vw/gfx/camera/free_camera_controller.h
export namespace vw::gfx {

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

// ---- from vw/gfx/camera/third_person_camera_controller.h
export namespace vw::gfx {


struct third_person_camera_params {
    float32 arm_length     = 10.0f;
    float32 arm_length_min = 2.0f;
    float32 arm_length_max = 50.0f;
    vec3f target_offset    = {0.0f, 8.0f, 0.0f};
    float32 pitch_min      = -89.0f;
    float32 pitch_max      = 89.0f;
    float32 zoom_speed     = 2.0f;
    float32 collision_skin = 0.3f;
};

/// Third-person camera that follows an entity with arm length, offset, and voxel collision.
class third_person_camera_controller {
public:
    using world_type = world;
    explicit third_person_camera_controller(
        camera& camera,
        world_type& world,
        third_person_camera_params params = {}
    );

    void update(const player_input_state& input, entity target);

    [[nodiscard]] auto get_params() -> third_person_camera_params&;
    [[nodiscard]] auto get_pitch() const -> float32;
    [[nodiscard]] auto get_yaw() const -> float32;
    [[nodiscard]] auto get_actual_arm_length() const -> float32;

private:
    camera* camera_;
    world_type* world_;
    third_person_camera_params params_;

    float32 pitch_ = 20.0f;
    float32 yaw_   = 0.0f;
    float32 actual_arm_length_ = 0.0f;
};

}  // namespace vw::gfx

// ---- from vw/gfx/player/player_input_controller.h
export namespace vw::gfx {


struct player_input_params {
    float32 mouse_sensitivity = 0.1f;
};

/// Unified input layer that reads keyboard/mouse and produces player_input_state.
class player_input_controller {
public:
    explicit player_input_controller(window& window, player_input_params params = {});
    ~player_input_controller();

    player_input_controller(const player_input_controller&)            = delete;
    auto operator=(const player_input_controller&) -> player_input_controller& = delete;
    player_input_controller(player_input_controller&&)                 = delete;
    auto operator=(player_input_controller&&) -> player_input_controller& = delete;

    [[nodiscard]] auto get_input_state() -> player_input_state;

    [[nodiscard]] auto get_params() -> player_input_params&;

    void set_mouse_captured(bool captured);
    [[nodiscard]] auto is_mouse_captured() const -> bool;

private:
    window* window_;
    player_input_params params_;

    bool mouse_captured_{false};

    double last_mouse_x_{0.0};
    double last_mouse_y_{0.0};
    bool mouse_initialized_{false};

    float32 accumulated_look_x_{0.0f};
    float32 accumulated_look_y_{0.0f};
    float32 accumulated_scroll_{0.0f};

    bool attack_pressed_{false};

    event_sub<mouse_move_event> mouse_move_sub_;
    event_sub<mouse_scroll_event> mouse_scroll_sub_;
    event_sub<mouse_press_event> mouse_press_sub_;
};

}  // namespace vw::gfx
