module vw.gfx;

import std;
import vw.core;
import vw.ecs;
import vw.world;
import vw.platform;

namespace vw::gfx {
camera::camera(
    float fov, float aspect, float near, float far
)
    : position_(0.0f, 0.0f, 0.0f)
    , pitch_(0.0f)
    , yaw_(0.0f)
    , fov_(fov)
    , aspect_(aspect)
    , near_(near)
    , far_(far)
    , forward_(0.0f, 0.0f, 1.0f)
    , right_(1.0f, 0.0f, 0.0f)
    , up_(0.0f, 1.0f, 0.0f)
    , vectors_dirty_(false)
    , view_matrix_dirty_(true)
    , projection_matrix_dirty_(true)
    , frustum_dirty_(true) {
    update_vectors();
}

auto camera::set_position(
    const vec3f& position
) -> void {
    position_          = position;
    view_matrix_dirty_ = true;
    frustum_dirty_     = true;
}

auto camera::set_rotation(
    float pitch, float yaw
) -> void {
    pitch_             = pitch;
    yaw_               = yaw;
    vectors_dirty_     = true;
    view_matrix_dirty_ = true;
    frustum_dirty_     = true;
}

auto camera::set_aspect_ratio(
    float aspect
) -> void {
    aspect_                  = aspect;
    projection_matrix_dirty_ = true;
    frustum_dirty_           = true;
}

auto camera::set_far(
    float far
) -> void {
    far_                     = far;
    projection_matrix_dirty_ = true;
    frustum_dirty_           = true;
}

auto camera::get_near() const -> float {
    return near_;
}

auto camera::get_far() const -> float {
    return far_;
}

auto camera::get_fov() const -> float {
    return fov_;
}

auto camera::get_aspect_ratio() const -> float {
    return aspect_;
}

auto camera::get_position() const -> vec3f {
    return position_;
}

auto camera::get_pitch() const -> float {
    return pitch_;
}

auto camera::get_yaw() const -> float {
    return yaw_;
}

auto camera::get_view_matrix() const -> mat4f {
    if (view_matrix_dirty_) {
        update_view_matrix();
    }
    return view_matrix_;
}

auto camera::get_projection_matrix() const -> mat4f {
    if (projection_matrix_dirty_) {
        update_projection_matrix();
    }
    return projection_matrix_;
}

auto camera::get_view_projection_matrix() const -> mat4f {
    return get_projection_matrix() * get_view_matrix();
}

auto camera::move_forward(
    float distance
) -> void {
    if (vectors_dirty_) {
        update_vectors();
    }
    position_          = position_ + forward_ * distance;
    view_matrix_dirty_ = true;
    frustum_dirty_     = true;
}

auto camera::move_right(
    float distance
) -> void {
    if (vectors_dirty_) {
        update_vectors();
    }
    position_          = position_ + right_ * distance;
    view_matrix_dirty_ = true;
    frustum_dirty_     = true;
}

auto camera::move_up(
    float distance
) -> void {
    if (vectors_dirty_) {
        update_vectors();
    }
    position_          = position_ + up_ * distance;
    view_matrix_dirty_ = true;
    frustum_dirty_     = true;
}

auto camera::rotate(
    float delta_pitch, float delta_yaw
) -> void {
    pitch_ += delta_pitch;
    yaw_ += delta_yaw;

    pitch_ = math::clamp(pitch_, -89.0f, 89.0f);

    vectors_dirty_     = true;
    view_matrix_dirty_ = true;
    frustum_dirty_     = true;
}

auto camera::get_forward() const -> vec3f {
    if (vectors_dirty_) {
        update_vectors();
    }
    return forward_;
}

auto camera::get_right() const -> vec3f {
    if (vectors_dirty_) {
        update_vectors();
    }
    return right_;
}

auto camera::get_up() const -> vec3f {
    if (vectors_dirty_) {
        update_vectors();
    }
    return up_;
}

auto camera::update_vectors() const -> void {
    const float pitch_rad = math::radians(pitch_);
    const float yaw_rad   = math::radians(yaw_);

    forward_.x = std::sin(yaw_rad) * std::cos(pitch_rad);
    forward_.y = std::sin(pitch_rad);
    forward_.z = std::cos(yaw_rad) * std::cos(pitch_rad);

    forward_ = math::normalize(forward_);
    right_   = math::normalize(math::cross(vec3f(0.0f, 1.0f, 0.0f), forward_));
    up_      = math::normalize(math::cross(forward_, right_));

    vectors_dirty_ = false;
}

auto camera::update_view_matrix() const -> void {
    if (vectors_dirty_) {
        update_vectors();
    }

    const vec3f center = position_ + forward_;
    view_matrix_       = math::look_at_matrix(position_, center, up_);
    view_matrix_dirty_ = false;
}

auto camera::update_projection_matrix() const -> void {
    projection_matrix_       = math::perspective_matrix_reversed(fov_, aspect_, near_, far_);
    projection_matrix_dirty_ = false;
}

auto camera::get_frustum() const -> const vw::spatial::frustum& {
    if (vectors_dirty_) {
        update_vectors();
    }
    if (view_matrix_dirty_) {
        update_view_matrix();
    }
    if (projection_matrix_dirty_) {
        update_projection_matrix();
    }
    if (frustum_dirty_) {
        update_frustum();
    }
    return frustum_;
}

auto camera::update_frustum() const -> void {
    frustum_       = vw::spatial::frustum::from_view_projection_matrix(get_view_projection_matrix());
    frustum_dirty_ = false;
}

auto camera::screen_to_world_ray(
    const vec2d& mouse_pos, const vec2i& window_size
) const -> vw::spatial::ray {
    if (window_size.x <= 0 || window_size.y <= 0) {
        return vw::spatial::ray{vec3f{0.0f, 0.0f, 0.0f}, vec3f{0.0f, 0.0f, 1.0f}};
    }

    const float ndc_x =
        static_cast<float>(mouse_pos.x) / static_cast<float>(window_size.x) * 2.0f - 1.0f;
    const float ndc_y =
        static_cast<float>(mouse_pos.y) / static_cast<float>(window_size.y) * 2.0f - 1.0f;

    const mat4f view_proj     = get_view_projection_matrix();
    const auto inv_result     = math::inverse_matrix(view_proj);
    const mat4f inv_view_proj = inv_result.value_or(math::identity_matrix());

    // Unproject точки на near vw::spatial::plane (ndc_z = -1.0)
    vec4f near_ndc{ndc_x, ndc_y, -1.0f, 1.0f};
    vec4f near_homogeneous = inv_view_proj * near_ndc;

    vec3f near_point;
    if (near_homogeneous.w != 0.0f && near_homogeneous.w != 1.0f) {
        near_point = vec3f{
            near_homogeneous.x / near_homogeneous.w,
            near_homogeneous.y / near_homogeneous.w,
            near_homogeneous.z / near_homogeneous.w
        };
    } else {
        near_point = vec3f{near_homogeneous.x, near_homogeneous.y, near_homogeneous.z};
    }

    // Unproject точки на far vw::spatial::plane (ndc_z = 1.0)
    vec4f far_ndc{ndc_x, ndc_y, 1.0f, 1.0f};
    vec4f far_homogeneous = inv_view_proj * far_ndc;

    vec3f far_point;
    if (far_homogeneous.w != 0.0f && far_homogeneous.w != 1.0f) {
        far_point = vec3f{
            far_homogeneous.x / far_homogeneous.w,
            far_homogeneous.y / far_homogeneous.w,
            far_homogeneous.z / far_homogeneous.w
        };
    } else {
        far_point = vec3f{far_homogeneous.x, far_homogeneous.y, far_homogeneous.z};
    }

    return vw::spatial::ray{near_point, far_point};
}
}  // namespace vw::gfx
