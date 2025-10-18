#pragma once

#ifndef VW_CORE_TRANSFORM_H
#define VW_CORE_TRANSFORM_H

#include "vw/core/math.h"

namespace vw {

struct transform {
private:
    vec3f position_{0.0F, 0.0F, 0.0F};
    vec3f rotation_{0.0F, 0.0F, 0.0F};  // углы в радианах
    vec3f scale_{1.0F, 1.0F, 1.0F};
    vec3f origin_{0.0F, 0.0F, 0.0F};

public:
    [[nodiscard]] auto get_position() const -> const vec3f&;
    [[nodiscard]] auto get_rotation() const -> const vec3f&;
    [[nodiscard]] auto get_scale() const -> const vec3f&;
    [[nodiscard]] auto get_origin() const -> const vec3f&;

    [[nodiscard]] auto calc_matrix() const -> mat4f;

    void set_position(const vec3f& position);
    void set_rotation(const vec3f& rotation);
    void set_scale(const vec3f& scale);
    void set_origin(const vec3f& origin);

    void translate(const vec3f& offset);
    void rotate(const vec3f& angles);
    void scale(const vec3f& factor);
};

}  // namespace vw

#include "vw/core/transform.inl.h"

#endif  // VW_CORE_TRANSFORM_H
