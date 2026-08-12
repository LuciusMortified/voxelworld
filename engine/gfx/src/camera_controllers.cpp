module vw.gfx;

import std;
import vw.core;
import vw.ecs;
import vw.world;
import vw.platform;

namespace vw::gfx {

fps_camera_controller::fps_camera_controller(float mouse_sensitivity, float camera_speed)
    : mouse_sensitivity_(mouse_sensitivity), camera_speed_(camera_speed) {}

void fps_camera_controller::setup(
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

void fps_camera_controller::update(
    float delta_time
) const {
    if (!camera_ || !window_)
        return;

    update_camera_movement_(delta_time);
}

void fps_camera_controller::set_mouse_sensitivity(
    float sensitivity
) {
    mouse_sensitivity_ = sensitivity;
}

float fps_camera_controller::get_mouse_sensitivity() const {
    return mouse_sensitivity_;
}

void fps_camera_controller::set_camera_speed(
    float speed
) {
    camera_speed_ = speed;
}

float fps_camera_controller::get_camera_speed() const {
    return camera_speed_;
}

bool fps_camera_controller::is_mouse_captured() const {
    return mouse_captured_;
}

void fps_camera_controller::handle_mouse_moved_(
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

void fps_camera_controller::update_camera_movement_(
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

void fps_camera_controller::set_mouse_captured(
    bool captured
) {
    mouse_captured_ = captured;

    window_->set_cursor_mode(mouse_captured_ ? cursor_modes::DISABLED : cursor_modes::NORMAL);
    window_->set_input_mode(input_modes::RAW_MOUSE_MOTION, mouse_captured_);

    mouse_initialized_ = false;
}

void fps_camera_controller::toggle_mouse_captured() {
    set_mouse_captured(!mouse_captured_);
}

bool fps_camera_controller::keyboard_control_enabled() const {
    return keyboard_control_enabled_;
}

void fps_camera_controller::set_keyboard_control_enabled(
    bool enabled
) {
    keyboard_control_enabled_ = enabled;
}

void fps_camera_controller::toggle_keyboard_control_enabled() {
    keyboard_control_enabled_ = !keyboard_control_enabled_;
}

}  // namespace vw::gfx

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

namespace vw::gfx {

third_person_camera_controller::third_person_camera_controller(
    camera& camera, world_type& world, third_person_camera_params params
)
    : camera_(&camera), world_(&world), params_(params), actual_arm_length_(params.arm_length) {}

void third_person_camera_controller::update(
    const player_input_state& input, entity target
) {
    yaw_ += input.look_delta.x;
    pitch_ += input.look_delta.y;
    pitch_ = math::clamp(pitch_, params_.pitch_min, params_.pitch_max);

    params_.arm_length -= input.zoom_delta * params_.zoom_speed;
    params_.arm_length =
        math::clamp(params_.arm_length, params_.arm_length_min, params_.arm_length_max);

    auto& registry = world_->registry();
    if (!registry.has<transform_component>(target)) {
        return;
    }

    const auto& tc        = registry.get<transform_component>(target);
    const auto player_pos = tc.get_position();
    const auto focus      = player_pos + params_.target_offset;

    const float32 yaw_rad   = math::radians(yaw_);
    const float32 pitch_rad = math::radians(pitch_);

    const vec3f arm_dir{
        std::sin(yaw_rad) * std::cos(pitch_rad),
        std::sin(pitch_rad),
        std::cos(yaw_rad) * std::cos(pitch_rad)
    };

    vec3f desired_pos = focus + arm_dir * params_.arm_length;

    actual_arm_length_ = params_.arm_length;

    auto& spatial_sys = world_->system<spatial_system>();
    vw::spatial::ray collision_ray{focus, desired_pos};
    std::vector<entity> candidates;
    constexpr spatial_layer_mask camera_mask = spatial_layer::terrain | spatial_layer::prop;
    auto hit = spatial_sys.voxel_ray_cast(collision_ray, candidates, camera_mask);

    if (hit) {
        const auto& hit_tc  = registry.get<transform_component>(hit->ent);
        vec3f hit_world_pos = hit_tc.get_world_matrix() *
            vec3f{
                static_cast<float32>(hit->voxel_pos.x) + 0.5f,
                static_cast<float32>(hit->voxel_pos.y) + 0.5f,
                static_cast<float32>(hit->voxel_pos.z) + 0.5f
            };

        float32 hit_distance = math::length(hit_world_pos - focus);
        float32 clamped      = hit_distance - params_.collision_skin;
        if (clamped < actual_arm_length_ && clamped > 0.0f) {
            actual_arm_length_ = clamped;
        }
    }

    vec3f cam_pos = focus + arm_dir * actual_arm_length_;
    camera_->set_position(cam_pos);

    auto look_dir           = focus - cam_pos;
    float32 horizontal_dist = std::sqrt(look_dir.x * look_dir.x + look_dir.z * look_dir.z);
    float32 look_pitch      = std::atan2(look_dir.y, horizontal_dist) * 180.0f / math::pi;
    float32 look_yaw        = std::atan2(look_dir.x, look_dir.z) * 180.0f / math::pi;
    camera_->set_rotation(look_pitch, look_yaw);
}

auto third_person_camera_controller::get_params() -> third_person_camera_params& {
    return params_;
}

auto third_person_camera_controller::get_pitch() const -> float32 {
    return pitch_;
}

auto third_person_camera_controller::get_yaw() const -> float32 {
    return yaw_;
}

auto third_person_camera_controller::get_actual_arm_length() const -> float32 {
    return actual_arm_length_;
}

}  // namespace vw::gfx

namespace vw::gfx {

player_input_controller::player_input_controller(
    window& window, player_input_params params
)
    : window_(&window)
    , params_(params)
    , mouse_move_sub_(0)
    , mouse_scroll_sub_(0)
    , mouse_press_sub_(0) {
    const auto cursor_pos = window_->get_cursor_pos();
    last_mouse_x_         = cursor_pos.x;
    last_mouse_y_         = cursor_pos.y;
    mouse_initialized_    = true;

    mouse_move_sub_ = window_->sub<mouse_move_event>([this](const mouse_move_event& event) -> bool {
        if (!mouse_captured_) {
            return false;
        }

        if (!mouse_initialized_) {
            last_mouse_x_      = event.x;
            last_mouse_y_      = event.y;
            mouse_initialized_ = true;
            return false;
        }

        const auto delta_x = event.x - last_mouse_x_;
        const auto delta_y = event.y - last_mouse_y_;
        last_mouse_x_      = event.x;
        last_mouse_y_      = event.y;

        accumulated_look_x_ += static_cast<float32>(delta_x) * params_.mouse_sensitivity;
        accumulated_look_y_ += static_cast<float32>(delta_y) * params_.mouse_sensitivity;

        return false;
    });

    mouse_scroll_sub_ =
        window_->sub<mouse_scroll_event>([this](const mouse_scroll_event& event) -> bool {
            accumulated_scroll_ += static_cast<float32>(event.offset_y);
            return false;
        });

    mouse_press_sub_ =
        window_->sub<mouse_press_event>([this](const mouse_press_event& event) -> bool {
            if (mouse_captured_ && event.button == mouse::buttons::LEFT) {
                attack_pressed_ = true;
            }
            return false;
        });
}

player_input_controller::~player_input_controller() {
    window_->unsub<mouse_move_event>(mouse_move_sub_);
    window_->unsub<mouse_scroll_event>(mouse_scroll_sub_);
    window_->unsub<mouse_press_event>(mouse_press_sub_);
}

auto player_input_controller::get_input_state() -> player_input_state {
    player_input_state state;

    if (window_->is_key_pressed(keyboard::keys::W)) {
        state.move_input.x += 1.0f;
    }
    if (window_->is_key_pressed(keyboard::keys::S)) {
        state.move_input.x -= 1.0f;
    }
    if (window_->is_key_pressed(keyboard::keys::D)) {
        state.move_input.y += 1.0f;
    }
    if (window_->is_key_pressed(keyboard::keys::A)) {
        state.move_input.y -= 1.0f;
    }

    const auto len_sq =  //
        state.move_input.x * state.move_input.x + state.move_input.y * state.move_input.y;
    if (len_sq > 0.0f) {
        const auto inv_len = 1.0f / std::sqrt(len_sq);
        state.move_input.x *= inv_len;
        state.move_input.y *= inv_len;
    }

    state.jump_requested   = window_->is_key_pressed(keyboard::keys::SPACE);
    state.sprint           = window_->is_key_pressed(keyboard::keys::LEFT_SHIFT);
    state.attack_requested = attack_pressed_;

    state.look_delta = {accumulated_look_x_, accumulated_look_y_};
    state.zoom_delta = accumulated_scroll_;

    accumulated_look_x_ = 0.0f;
    accumulated_look_y_ = 0.0f;
    accumulated_scroll_ = 0.0f;
    attack_pressed_     = false;

    return state;
}

auto player_input_controller::get_params() -> player_input_params& {
    return params_;
}

void player_input_controller::set_mouse_captured(
    bool captured
) {
    mouse_captured_ = captured;
    window_->set_cursor_mode(mouse_captured_ ? cursor_modes::DISABLED : cursor_modes::NORMAL);
    window_->set_input_mode(input_modes::RAW_MOUSE_MOTION, mouse_captured_);
    mouse_initialized_ = false;
}

auto player_input_controller::is_mouse_captured() const -> bool {
    return mouse_captured_;
}

}  // namespace vw::gfx
