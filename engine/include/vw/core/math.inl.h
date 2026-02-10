#pragma once

#ifndef VW_CORE_MATH_INL_H
#define VW_CORE_MATH_INL_H
#include <limits>

namespace vw::math {

inline float radians(
    float degrees
) {
    return degrees * deg_to_rad;
}

inline float degrees(
    float radians
) {
    return radians * rad_to_deg;
}

inline float length(
    const vec3f& v
) {
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

inline float length_squared(
    const vec3f& v
) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

inline vec3f normalize(
    const vec3f& v
) {
    float len = length(v);
    if (len > 0.0f) {
        return {v.x / len, v.y / len, v.z / len};
    }
    return v;
}

inline vec3f cross(
    const vec3f& a, const vec3f& b
) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

inline float dot(
    const vec3f& a, const vec3f& b
) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline mat4f perspective_matrix(
    float fov, float aspect, float near, float far
) {
    mat4f matrix  = identity_matrix();
    const float f = 1.0f / std::tan(radians(fov * 0.5f));

    matrix[0, 0] = f / aspect;
    matrix[1, 1] = -f;
    matrix[2, 2] = (far + near) / (near - far);
    matrix[2, 3] = (2.0f * far * near) / (near - far);
    matrix[3, 2] = -1.0f;
    matrix[3, 3] = 0.0f;

    return matrix;
}

inline mat4f orthographic_matrix(
    float left, float right, float bottom, float top, float near, float far
) {
    mat4f matrix = identity_matrix();

    matrix[0, 0] = 2.0f / (right - left);
    matrix[1, 1] = 2.0f / (top - bottom);
    matrix[2, 2] = -1.0f / (far - near);
    matrix[3, 0] = -(right + left) / (right - left);
    matrix[3, 1] = -(top + bottom) / (top - bottom);
    matrix[3, 2] = -near / (far - near);

    return matrix;
}

inline mat4f look_at_matrix(
    const vec3f& eye, const vec3f& target
) {
    mat4f matrix  = identity_matrix();
    const vec3f f = normalize(target - eye);
    auto up       = vec3f{0.0f, 1.0f, 0.0f};
    if (std::abs(dot(f, up)) > 0.999f) {
        up = vec3f{1.0f, 0.0f, 0.0f};
    }

    const vec3f s = normalize(cross(up, f));
    const vec3f u = cross(f, s);

    matrix[0, 0] = s.x;
    matrix[0, 1] = s.y;
    matrix[0, 2] = s.z;
    matrix[0, 3] = -dot(s, eye);

    matrix[1, 0] = u.x;
    matrix[1, 1] = u.y;
    matrix[1, 2] = u.z;
    matrix[1, 3] = -dot(u, eye);

    matrix[2, 0] = -f.x;
    matrix[2, 1] = -f.y;
    matrix[2, 2] = -f.z;
    matrix[2, 3] = dot(f, eye);

    return matrix;
}

inline mat4f look_at_matrix(
    const vec3f& eye, const vec3f& target, const vec3f& up
) {
    mat4f matrix  = identity_matrix();
    const vec3f f = normalize(target - eye);
    const vec3f s = normalize(cross(up, f));
    const vec3f u = cross(f, s);

    matrix[0, 0] = s.x;
    matrix[0, 1] = s.y;
    matrix[0, 2] = s.z;
    matrix[0, 3] = -dot(s, eye);

    matrix[1, 0] = u.x;
    matrix[1, 1] = u.y;
    matrix[1, 2] = u.z;
    matrix[1, 3] = -dot(u, eye);

    matrix[2, 0] = -f.x;
    matrix[2, 1] = -f.y;
    matrix[2, 2] = -f.z;
    matrix[2, 3] = dot(f, eye);

    return matrix;
}

inline mat4f identity_matrix() {
    mat4f matrix;
    matrix[0, 0] = 1.0f;
    matrix[1, 1] = 1.0f;
    matrix[2, 2] = 1.0f;
    matrix[3, 3] = 1.0f;
    return matrix;
}

inline mat4f translation_matrix(
    const vec3f& translation
) {
    mat4f matrix = identity_matrix();
    matrix[0, 3] = translation.x;
    matrix[1, 3] = translation.y;
    matrix[2, 3] = translation.z;
    return matrix;
}

inline mat4f rotation_matrix_x(
    float angle
) {
    mat4f matrix  = identity_matrix();
    const float c = std::cos(angle);
    const float s = std::sin(angle);

    matrix[1, 1] = c;
    matrix[1, 2] = s;
    matrix[2, 1] = -s;
    matrix[2, 2] = c;

    return matrix;
}

inline mat4f rotation_matrix_y(
    float angle
) {
    mat4f matrix  = identity_matrix();
    const float c = std::cos(angle);
    const float s = std::sin(angle);

    matrix[0, 0] = c;
    matrix[0, 2] = -s;
    matrix[2, 0] = s;
    matrix[2, 2] = c;

    return matrix;
}

inline mat4f rotation_matrix_z(
    float angle
) {
    mat4f matrix  = identity_matrix();
    const float c = std::cos(angle);
    const float s = std::sin(angle);

    matrix[0, 0] = c;
    matrix[0, 1] = s;
    matrix[1, 0] = -s;
    matrix[1, 1] = c;

    return matrix;
}

inline mat4f rotation_matrix(
    const vec3f& rotation
) {
    const mat4f rot_x = rotation_matrix_x(rotation.x);
    const mat4f rot_y = rotation_matrix_y(rotation.y);
    const mat4f rot_z = rotation_matrix_z(rotation.z);

    return rot_z * rot_y * rot_x;
}

inline mat4f scale_matrix(
    const vec3f& scale
) {
    mat4f matrix = identity_matrix();
    matrix[0, 0] = scale.x;
    matrix[1, 1] = scale.y;
    matrix[2, 2] = scale.z;
    return matrix;
}

inline mat4f transform_matrix(
    const vec3f& position, const vec3f& rotation, const vec3f& scale, const vec3f& origin
) {
    const mat4f trans     = translation_matrix(position);
    const mat4f orig_back = translation_matrix(origin);
    const mat4f rot       = rotation_matrix(rotation);
    const mat4f scl       = scale_matrix(scale);

    return trans * rot * scl * orig_back;
}

inline mat4f transpose_matrix(
    const mat4f& matrix
) {
    mat4f result;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result[i, j] = matrix[j, i];
        }
    }
    return result;
}

inline float clamp(
    float value, float min_val, float max_val
) {
    if (value < min_val)
        return min_val;
    if (value > max_val)
        return max_val;
    return value;
}

inline float lerp(
    float a, float b, float t
) {
    return a + t * (b - a);
}

inline vec3f lerp(
    const vec3f& a, const vec3f& b, float t
) {
    return {lerp(a.x, b.x, t), lerp(a.y, b.y, t), lerp(a.z, b.z, t)};
}

inline transform lerp(
    const transform& a, const transform& b, float t
) {
    transform result;
    result.set_position(lerp(a.get_position(), b.get_position(), t));
    result.set_rotation(lerp(a.get_rotation(), b.get_rotation(), t));
    result.set_scale(lerp(a.get_scale(), b.get_scale(), t));
    result.set_origin(lerp(a.get_origin(), b.get_origin(), t));
    return result;
}

inline vec3f ease_in(const vec3f& a, const vec3f& b, float t) {
    float eased_t = t * t;
    return lerp(a, b, eased_t);
}

inline vec3f ease_out(const vec3f& a, const vec3f& b, float t) {
    float eased_t = t * (2.0f - t);
    return lerp(a, b, eased_t);
}

inline vec3f ease_in_out(const vec3f& a, const vec3f& b, float t) {
    float eased_t;
    if (t < 0.5f) {
        eased_t = 2.0f * t * t;
    } else {
        eased_t = -1.0f + (4.0f - 2.0f * t) * t;
    }
    return lerp(a, b, eased_t);
}

inline float evaluate_cubic_bezier(float t, float p0, float p1, float p2, float p3) {
    float one_minus_t = 1.0f - t;
    float one_minus_t_sq = one_minus_t * one_minus_t;
    float one_minus_t_cb = one_minus_t_sq * one_minus_t;
    float t_sq = t * t;
    float t_cb = t_sq * t;

    return one_minus_t_cb * p0 + 3.0f * one_minus_t_sq * t * p1 +
           3.0f * one_minus_t * t_sq * p2 + t_cb * p3;
}

inline uint8 lerp(uint8 a, uint8 b, float t) {
    return static_cast<uint8>(a + t * (b - a));
}

inline vec3f cubic_bezier(
    const vec3f& a,
    const vec3f& b,
    float t,
    float control1,
    float control2
) {
    float bezier_t = evaluate_cubic_bezier(t, 0.0f, control1, control2, 1.0f);
    return lerp(a, b, bezier_t);
}

inline float apply_easing(float t, interpolation_type type) {
    t = clamp(t, 0.0f, 1.0f);

    switch (type) {
        case interpolation_type::linear:
            return t;

        case interpolation_type::step:
            return t < 1.0f ? 0.0f : 1.0f;

        case interpolation_type::ease_in:
            return t * t;

        case interpolation_type::ease_out:
            return t * (2.0f - t);

        case interpolation_type::ease_in_out:
            if (t < 0.5f) {
                return 2.0f * t * t;
            } else {
                return -1.0f + (4.0f - 2.0f * t) * t;
            }

        case interpolation_type::cubic_bezier:
            return t;

        default:
            return t;
    }
}

inline float apply_easing_bezier(
    float t,
    interpolation_type type,
    float control1,
    float control2
) {
    if (type == interpolation_type::cubic_bezier) {
        t = clamp(t, 0.0f, 1.0f);
        return evaluate_cubic_bezier(t, 0.0f, control1, control2, 1.0f);
    } else {
        return apply_easing(t, type);
    }
}

inline vec3f interpolate(
    const vec3f& a,
    const vec3f& b,
    float t,
    interpolation_type type,
    float control1,
    float control2
) {
    switch (type) {
        case interpolation_type::linear:
            return lerp(a, b, t);

        case interpolation_type::step:
            return t < 1.0f ? a : b;

        case interpolation_type::ease_in:
            return ease_in(a, b, t);

        case interpolation_type::ease_out:
            return ease_out(a, b, t);

        case interpolation_type::ease_in_out:
            return ease_in_out(a, b, t);

        case interpolation_type::cubic_bezier:
            return cubic_bezier(a, b, t, control1, control2);

        default:
            return lerp(a, b, t);
    }
}

inline color interpolate(
    color a,
    color b,
    float t,
    interpolation_type type,
    float control1,
    float control2
) {
    if (type == interpolation_type::step) {
        return t < 1.0f ? a : b;
    }

    float eased_t = apply_easing_bezier(t, type, control1, control2);

    uint8 r = lerp(a.r(), b.r(), eased_t);
    uint8 g = lerp(a.g(), b.g(), eased_t);
    uint8 b_c = lerp(a.b(), b.b(), eased_t);
    uint8 a_c = lerp(a.a(), b.a(), eased_t);

    return color(r, g, b_c, a_c);
}

inline vec3f perpendicular(
    const vec3f& eye, const vec3f& target
) {
    auto dir = normalize(target - eye);
    auto up  = vec3f{0.0f, 1.0f, 0.0f};

    if (std::abs(dot(dir, up)) > 0.999f) {
        up = vec3f{1.0f, 0.0f, 0.0f};
    }

    return normalize(cross(up, dir));
}

inline bool is_safe_zero(
    float a, float b, float epsilon
) {
    return std::abs(a - b) < epsilon;
}

inline mat4f inverse_matrix(
    const mat4f& matrix
) {
    mat4f inv;

    inv[0, 0] =  //
        matrix[1, 1] * (matrix[2, 2] * matrix[3, 3] - matrix[2, 3] * matrix[3, 2]) -
        matrix[1, 2] * (matrix[2, 1] * matrix[3, 3] - matrix[2, 3] * matrix[3, 1]) +
        matrix[1, 3] * (matrix[2, 1] * matrix[3, 2] - matrix[2, 2] * matrix[3, 1]);

    inv[0, 1] =  //
        -(matrix[0, 1] * (matrix[2, 2] * matrix[3, 3] - matrix[2, 3] * matrix[3, 2]) -
          matrix[0, 2] * (matrix[2, 1] * matrix[3, 3] - matrix[2, 3] * matrix[3, 1]) +
          matrix[0, 3] * (matrix[2, 1] * matrix[3, 2] - matrix[2, 2] * matrix[3, 1]));

    inv[0, 2] =  //
        matrix[0, 1] * (matrix[1, 2] * matrix[3, 3] - matrix[1, 3] * matrix[3, 2]) -
        matrix[0, 2] * (matrix[1, 1] * matrix[3, 3] - matrix[1, 3] * matrix[3, 1]) +
        matrix[0, 3] * (matrix[1, 1] * matrix[3, 2] - matrix[1, 2] * matrix[3, 1]);

    inv[0, 3] =  //
        -(matrix[0, 1] * (matrix[1, 2] * matrix[2, 3] - matrix[1, 3] * matrix[2, 2]) -
          matrix[0, 2] * (matrix[1, 1] * matrix[2, 3] - matrix[1, 3] * matrix[2, 1]) +
          matrix[0, 3] * (matrix[1, 1] * matrix[2, 2] - matrix[1, 2] * matrix[2, 1]));

    inv[1, 0] =  //
        -(matrix[1, 0] * (matrix[2, 2] * matrix[3, 3] - matrix[2, 3] * matrix[3, 2]) -
          matrix[1, 2] * (matrix[2, 0] * matrix[3, 3] - matrix[2, 3] * matrix[3, 0]) +
          matrix[1, 3] * (matrix[2, 0] * matrix[3, 2] - matrix[2, 2] * matrix[3, 0]));

    inv[1, 1] =  //
        matrix[0, 0] * (matrix[2, 2] * matrix[3, 3] - matrix[2, 3] * matrix[3, 2]) -
        matrix[0, 2] * (matrix[2, 0] * matrix[3, 3] - matrix[2, 3] * matrix[3, 0]) +
        matrix[0, 3] * (matrix[2, 0] * matrix[3, 2] - matrix[2, 2] * matrix[3, 0]);

    inv[1, 2] =  //
        -(matrix[0, 0] * (matrix[1, 2] * matrix[3, 3] - matrix[1, 3] * matrix[3, 2]) -
          matrix[0, 2] * (matrix[1, 0] * matrix[3, 3] - matrix[1, 3] * matrix[3, 0]) +
          matrix[0, 3] * (matrix[1, 0] * matrix[3, 2] - matrix[1, 2] * matrix[3, 0]));

    inv[1, 3] =  //
        matrix[0, 0] * (matrix[1, 2] * matrix[2, 3] - matrix[1, 3] * matrix[2, 2]) -
        matrix[0, 2] * (matrix[1, 0] * matrix[2, 3] - matrix[1, 3] * matrix[2, 0]) +
        matrix[0, 3] * (matrix[1, 0] * matrix[2, 2] - matrix[1, 2] * matrix[2, 0]);

    inv[2, 0] =  //
        matrix[1, 0] * (matrix[2, 1] * matrix[3, 3] - matrix[2, 3] * matrix[3, 1]) -
        matrix[1, 1] * (matrix[2, 0] * matrix[3, 3] - matrix[2, 3] * matrix[3, 0]) +
        matrix[1, 3] * (matrix[2, 0] * matrix[3, 1] - matrix[2, 1] * matrix[3, 0]);

    inv[2, 1] =  //
        -(matrix[0, 0] * (matrix[2, 1] * matrix[3, 3] - matrix[2, 3] * matrix[3, 1]) -
          matrix[0, 1] * (matrix[2, 0] * matrix[3, 3] - matrix[2, 3] * matrix[3, 0]) +
          matrix[0, 3] * (matrix[2, 0] * matrix[3, 1] - matrix[2, 1] * matrix[3, 0]));

    inv[2, 2] =  //
        matrix[0, 0] * (matrix[1, 1] * matrix[3, 3] - matrix[1, 3] * matrix[3, 1]) -
        matrix[0, 1] * (matrix[1, 0] * matrix[3, 3] - matrix[1, 3] * matrix[3, 0]) +
        matrix[0, 3] * (matrix[1, 0] * matrix[3, 1] - matrix[1, 1] * matrix[3, 0]);

    inv[2, 3] =  //
        -(matrix[0, 0] * (matrix[1, 1] * matrix[2, 3] - matrix[1, 3] * matrix[2, 1]) -
          matrix[0, 1] * (matrix[1, 0] * matrix[2, 3] - matrix[1, 3] * matrix[2, 0]) +
          matrix[0, 3] * (matrix[1, 0] * matrix[2, 1] - matrix[1, 1] * matrix[2, 0]));

    inv[3, 0] =  //
        -(matrix[1, 0] * (matrix[2, 1] * matrix[3, 2] - matrix[2, 2] * matrix[3, 1]) -
          matrix[1, 1] * (matrix[2, 0] * matrix[3, 2] - matrix[2, 2] * matrix[3, 0]) +
          matrix[1, 2] * (matrix[2, 0] * matrix[3, 1] - matrix[2, 1] * matrix[3, 0]));

    inv[3, 1] =  //
        matrix[0, 0] * (matrix[2, 1] * matrix[3, 2] - matrix[2, 2] * matrix[3, 1]) -
        matrix[0, 1] * (matrix[2, 0] * matrix[3, 2] - matrix[2, 2] * matrix[3, 0]) +
        matrix[0, 2] * (matrix[2, 0] * matrix[3, 1] - matrix[2, 1] * matrix[3, 0]);

    inv[3, 2] =  //
        -(matrix[0, 0] * (matrix[1, 1] * matrix[3, 2] - matrix[1, 2] * matrix[3, 1]) -
          matrix[0, 1] * (matrix[1, 0] * matrix[3, 2] - matrix[1, 2] * matrix[3, 0]) +
          matrix[0, 2] * (matrix[1, 0] * matrix[3, 1] - matrix[1, 1] * matrix[3, 0]));

    inv[3, 3] =  //
        matrix[0, 0] * (matrix[1, 1] * matrix[2, 2] - matrix[1, 2] * matrix[2, 1]) -
        matrix[0, 1] * (matrix[1, 0] * matrix[2, 2] - matrix[1, 2] * matrix[2, 0]) +
        matrix[0, 2] * (matrix[1, 0] * matrix[2, 1] - matrix[1, 1] * matrix[2, 0]);

    float det =                     //
        matrix[0, 0] * inv[0, 0] +  //
        matrix[0, 1] * inv[1, 0] +  //
        matrix[0, 2] * inv[2, 0] +  //
        matrix[0, 3] * inv[3, 0];

    if (std::abs(det) < std::numeric_limits<float>::epsilon()) {
        return identity_matrix();
    }

    float inv_det = 1.0f / det;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            inv[i, j] *= inv_det;
        }
    }

    return inv;
}

}  // namespace vw::math

#endif  // VW_CORE_MATH_INL_H
