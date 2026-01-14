#pragma once

#ifndef VW_GFX_CAMERA_INL_H
#define VW_GFX_CAMERA_INL_H

namespace vw::gfx {
inline camera::camera(
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

inline void camera::set_position(
    const vec3f& position
) {
    position_          = position;
    view_matrix_dirty_ = true;
    frustum_dirty_     = true;
}

inline void camera::set_rotation(
    float pitch, float yaw
) {
    pitch_             = pitch;
    yaw_               = yaw;
    vectors_dirty_     = true;
    view_matrix_dirty_ = true;
    frustum_dirty_     = true;
}

inline void camera::set_aspect_ratio(
    float aspect
) {
    aspect_                  = aspect;
    projection_matrix_dirty_ = true;
    frustum_dirty_           = true;
}

inline float camera::get_near() const {
    return near_;
}

inline float camera::get_far() const {
    return far_;
}

inline vec3f camera::get_position() const  {
    return position_;
}

inline float camera::get_pitch() const {
    return pitch_;
}

inline float camera::get_yaw() const  {
    return yaw_;
}

inline mat4f camera::get_view_matrix() const {
    if (view_matrix_dirty_) {
        update_view_matrix();
    }
    return view_matrix_;
}

inline mat4f camera::get_projection_matrix() const {
    if (projection_matrix_dirty_) {
        update_projection_matrix();
    }
    return projection_matrix_;
}

inline mat4f camera::get_view_projection_matrix() const {
    return get_projection_matrix() * get_view_matrix();
}

inline void camera::move_forward(
    float distance
) {
    if (vectors_dirty_) {
        update_vectors();
    }
    position_          = position_ + forward_ * distance;
    view_matrix_dirty_ = true;
    frustum_dirty_     = true;
}

inline void camera::move_right(
    float distance
) {
    if (vectors_dirty_) {
        update_vectors();
    }
    position_          = position_ + right_ * distance;
    view_matrix_dirty_ = true;
    frustum_dirty_     = true;
}

inline void camera::move_up(
    float distance
) {
    if (vectors_dirty_) {
        update_vectors();
    }
    position_          = position_ + up_ * distance;
    view_matrix_dirty_ = true;
    frustum_dirty_     = true;
}

inline void camera::rotate(
    float delta_pitch, float delta_yaw
) {
    pitch_ += delta_pitch;
    yaw_ += delta_yaw;

    pitch_ = math::clamp(pitch_, -89.0f, 89.0f);

    vectors_dirty_     = true;
    view_matrix_dirty_ = true;
    frustum_dirty_     = true;
}

inline vec3f camera::get_forward() const {
    if (vectors_dirty_) {
        update_vectors();
    }
    return forward_;
}

inline vec3f camera::get_right() const {
    if (vectors_dirty_) {
        update_vectors();
    }
    return right_;
}

inline vec3f camera::get_up() const {
    if (vectors_dirty_) {
        update_vectors();
    }
    return up_;
}

inline void camera::update_vectors() const {
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

inline void camera::update_view_matrix() const {
    if (vectors_dirty_) {
        update_vectors();
    }

    const vec3f center = position_ + forward_;
    view_matrix_       = math::look_at_matrix(position_, center, up_);
    view_matrix_dirty_ = false;
}

inline void camera::update_projection_matrix() const {
    projection_matrix_       = math::perspective_matrix(fov_, aspect_, near_, far_);
    projection_matrix_dirty_ = false;
}

inline const frustum& camera::get_frustum() const {
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

inline void camera::update_frustum() const {
    frustum_ = frustum::from_view_projection_matrix(get_view_projection_matrix());
    frustum_dirty_ = false;
}

inline auto camera::screen_to_world_ray(
    const vec2d& mouse_pos,
    const vec2i& window_size
) const -> ray {
    // Проверка валидности размера окна
    if (window_size.x <= 0 || window_size.y <= 0) {
        // Возвращаем нулевой луч в случае невалидного размера
        return ray{vec3f{0.0f, 0.0f, 0.0f}, vec3f{0.0f, 0.0f, 1.0f}};
    }

    // Преобразование screen coordinates в NDC (Normalized Device Coordinates)
    const float ndc_x = (static_cast<float>(mouse_pos.x) / static_cast<float>(window_size.x)) * 2.0f - 1.0f;
    const float ndc_y = (static_cast<float>(mouse_pos.y) / static_cast<float>(window_size.y)) * 2.0f - 1.0f;

    // Получить view-projection матрицу и обратную к ней
    const mat4f view_proj      = get_view_projection_matrix();
    const mat4f inverse_view_proj = vw::math::inverse_matrix(view_proj);

    // Unproject точки на near plane (ndc_z = -1.0)
    vec4f near_ndc{ndc_x, ndc_y, -1.0f, 1.0f};
    vec4f near_homogeneous = inverse_view_proj * near_ndc;
    
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

    // Unproject точки на far plane (ndc_z = 1.0)
    vec4f far_ndc{ndc_x, ndc_y, 1.0f, 1.0f};
    vec4f far_homogeneous = inverse_view_proj * far_ndc;
    
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

    // Создать и вернуть луч от near до far точки
    return ray{near_point, far_point};
}
}  // namespace vw::gfx

#endif  // VW_GFX_CAMERA_INL_H
