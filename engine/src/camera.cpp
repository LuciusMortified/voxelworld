#include <voxel/camera.h>
#include <voxel/math_utils.h>

namespace voxel {

camera::camera(
    float fov,
    float aspect,
    float near,
    float far
) : position_(0.0f, 0.0f, 0.0f), pitch_(0.0f), yaw_(-90.0f),
    fov_(fov), aspect_(aspect), near_(near), far_(far),
    forward_(0.0f, 0.0f, -1.0f), right_(1.0f, 0.0f, 0.0f), up_(0.0f, 1.0f, 0.0f),
    vectors_dirty_(false), view_matrix_(), projection_matrix_(),
    view_matrix_dirty_(true), projection_matrix_dirty_(true) {
    update_vectors();
}

void camera::set_position(const vec3f& position) {
    position_ = position;
    view_matrix_dirty_ = true;
}

void camera::set_rotation(float pitch, float yaw) {
    pitch_ = pitch;
    yaw_ = yaw;
    vectors_dirty_ = true;
    view_matrix_dirty_ = true;
}

void camera::set_aspect_ratio(float aspect) {
    aspect_ = aspect;
    projection_matrix_dirty_ = true;
}

mat4f camera::get_view_matrix() const {
    if (view_matrix_dirty_) {
        update_view_matrix();
    }
    return view_matrix_;
}

mat4f camera::get_projection_matrix() const {
    if (projection_matrix_dirty_) {
        update_projection_matrix();
    }
    return projection_matrix_;
}

mat4f camera::get_view_projection_matrix() const {
    return math::multiply_matrices(get_projection_matrix(), get_view_matrix());
}

void camera::move_forward(float distance) {
    if (vectors_dirty_) {
        update_vectors();
    }
    position_ = position_ + forward_ * distance;
    view_matrix_dirty_ = true;
}

void camera::move_right(float distance) {
    if (vectors_dirty_) {
        update_vectors();
    }
    position_ = position_ + right_ * distance;
    view_matrix_dirty_ = true;
}

void camera::move_up(float distance) {
    if (vectors_dirty_) {
        update_vectors();
    }
    position_ = position_ + up_ * distance;
    view_matrix_dirty_ = true;
}

void camera::rotate(float delta_pitch, float delta_yaw) {
    pitch_ += delta_pitch;
    yaw_ += delta_yaw;
    
    // Ограничиваем pitch чтобы избежать переворота камеры
    if (pitch_ > 89.0f) pitch_ = 89.0f;
    if (pitch_ < -89.0f) pitch_ = -89.0f;
    
    vectors_dirty_ = true;
    view_matrix_dirty_ = true;
}

vec3f camera::get_forward() const {
    if (vectors_dirty_) {
        update_vectors();
    }
    return forward_;
}

vec3f camera::get_right() const {
    if (vectors_dirty_) {
        update_vectors();
    }
    return right_;
}

vec3f camera::get_up() const {
    if (vectors_dirty_) {
        update_vectors();
    }
    return up_;
}

void camera::update_vectors() const {
    float pitch_rad = math::radians(pitch_);
    float yaw_rad = math::radians(yaw_);
    
    forward_.x = std::cos(yaw_rad) * std::cos(pitch_rad);
    forward_.y = std::sin(pitch_rad);
    forward_.z = std::sin(yaw_rad) * std::cos(pitch_rad);
    
    forward_ = math::normalize(forward_);
    right_ = math::normalize(math::cross(forward_, vec3f(0.0f, 1.0f, 0.0f)));
    up_ = math::normalize(math::cross(right_, forward_));
    
    vectors_dirty_ = false;
}

void camera::update_view_matrix() const {
    if (vectors_dirty_) {
        update_vectors();
    }
    
    vec3f center = position_ + forward_;
    view_matrix_ = math::look_at_matrix(position_, center, up_);
    view_matrix_dirty_ = false;
}

void camera::update_projection_matrix() const {
    projection_matrix_ = math::perspective_matrix(fov_, aspect_, near_, far_);
    projection_matrix_dirty_ = false;
}

} // namespace voxel 