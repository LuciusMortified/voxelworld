#include "vw/core/transform.h"
#include "vw/core/math.h"

namespace vw {

const mat4f& transform::get_matrix() const {
    if (matrix_dirty_) {
        cached_matrix_ = math::transform_matrix(position_, rotation_, scale_);
        matrix_dirty_  = false;
    }
    return cached_matrix_;
}

void transform::set_position(const vec3f& pos) {
    position_ = pos;
    mark_dirty();
}

void transform::set_rotation(const vec3f& rot) {
    rotation_ = rot;
    mark_dirty();
}

void transform::set_scale(const vec3f& scl) {
    scale_ = scl;
    mark_dirty();
}

void transform::translate(const vec3f& offset) {
    position_.x += offset.x;
    position_.y += offset.y;
    position_.z += offset.z;
    mark_dirty();
}

void transform::rotate(const vec3f& angles) {
    rotation_.x += angles.x;
    rotation_.y += angles.y;
    rotation_.z += angles.z;
    mark_dirty();
}

void transform::scale(const vec3f& factor) {
    scale_.x *= factor.x;
    scale_.y *= factor.y;
    scale_.z *= factor.z;
    mark_dirty();
}

}  // namespace vw
