#pragma once

#ifndef VW_CORE_TRANSFORM_H
#define VW_CORE_TRANSFORM_H

#include "vw/core.h"

namespace vw {

struct transform {
private:
    vec3f position_{0.0f, 0.0f, 0.0f};
    vec3f rotation_{0.0f, 0.0f, 0.0f};  // углы в радианах
    vec3f scale_{1.0f, 1.0f, 1.0f};

    mutable mat4f cached_matrix_;
    mutable bool matrix_dirty_ = true;

public:
    const vec3f& get_position() const {
        return position_;
    }

    const vec3f& get_rotation() const {
        return rotation_;
    }

    const vec3f& get_scale() const {
        return scale_;
    }

    const mat4f& get_matrix() const;

    void set_position(const vec3f& pos);
    void set_rotation(const vec3f& rot);
    void set_scale(const vec3f& scl);

    void translate(const vec3f& offset);
    void rotate(const vec3f& angles);
    void scale(const vec3f& factor);

    void mark_dirty() const {
        matrix_dirty_ = true;
    }
};

}  // namespace vw

#endif  // VW_CORE_TRANSFORM_H
