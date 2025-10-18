#pragma once

#ifndef VW_CORE_TRANSFORM_INL_H
#define VW_CORE_TRANSFORM_INL_H

#include "vw/core/math.h"
#include "vw/core/transform.h"
#include "vw/core/vec3.h"

namespace vw {

inline auto transform::get_position() const -> const vec3f& {
    return position_;
}

inline auto transform::get_rotation() const -> const vec3f& {
    return rotation_;
}

inline auto transform::get_scale() const -> const vec3f& {
    return scale_;
}

inline auto transform::get_origin() const -> const vec3f& {
    return origin_;
}

inline auto transform::calc_matrix() const -> mat4f {
    return math::transform_matrix(position_, rotation_, scale_, origin_);
}

inline void transform::set_position(const vec3f& position) {
    position_ = position;
}

inline void transform::set_rotation(const vec3f& rotation) {
    rotation_ = rotation;
}

inline void transform::set_scale(const vec3f& scale) {
    scale_ = scale;
}

inline void transform::set_origin(const vec3f& origin) {
    origin_ = origin;
}

inline void transform::translate(const vec3f& offset) {
    position_ += offset;
}

inline void transform::rotate(const vec3f& angles) {
    rotation_ += angles;
}

inline void transform::scale(const vec3f& factor) {
    scale_.x *= factor.x;
    scale_.y *= factor.y;
    scale_.z *= factor.z;
}

}  // namespace vw

#endif  // VW_CORE_TRANSFORM_INL_H
