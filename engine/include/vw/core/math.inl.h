#pragma once

#ifndef VW_CORE_MATH_INL_H
#define VW_CORE_MATH_INL_H

#include <cmath>
#include <limits>

#include "vw/core/transform.h"

namespace vw::math {

constexpr float radians(
    float degrees
) {
    return degrees * deg_to_rad;
}

constexpr float degrees(
    float radians
) {
    return radians * rad_to_deg;
}

template <typename T>
constexpr T clamp(
    T value, T min_val, T max_val
) {
    if (value < min_val)
        return min_val;
    if (value > max_val)
        return max_val;
    return value;
}

template <typename T>
constexpr T sign(
    T value
) {
    return value > T(0) ? T(1) : value < T(0) ? T(-1) : T(0);
}

inline bool approx_equal(
    float a, float b, float epsilon
) {
    return std::abs(a - b) < epsilon;
}

inline bool approx_equal(
    const vec2f& a, const vec2f& b, float epsilon
) {
    return approx_equal(a.x, b.x, epsilon) && approx_equal(a.y, b.y, epsilon);
}

inline bool approx_equal(
    const vec3f& a, const vec3f& b, float epsilon
) {
    return approx_equal(a.x, b.x, epsilon) && approx_equal(a.y, b.y, epsilon) &&
        approx_equal(a.z, b.z, epsilon);
}

inline bool approx_equal(
    const vec4f& a, const vec4f& b, float epsilon
) {
    return approx_equal(a.x, b.x, epsilon) && approx_equal(a.y, b.y, epsilon) &&
        approx_equal(a.z, b.z, epsilon) && approx_equal(a.w, b.w, epsilon);
}

inline bool approx_equal(
    const mat4f& a, const mat4f& b, float epsilon
) {
    for (int i = 0; i < 16; ++i) {
        if (!approx_equal(a[i], b[i], epsilon)) {
            return false;
        }
    }
    return true;
}

inline bool approx_equal(
    const quat& a, const quat& b, float epsilon
) {
    return approx_equal(a.x, b.x, epsilon) && approx_equal(a.y, b.y, epsilon) &&
        approx_equal(a.z, b.z, epsilon) && approx_equal(a.w, b.w, epsilon);
}

inline float length(
    const vec2f& v
) {
    return std::sqrt(v.x * v.x + v.y * v.y);
}

inline float length_squared(
    const vec2f& v
) {
    return v.x * v.x + v.y * v.y;
}

inline vec2f normalize(
    const vec2f& v
) {
    float len = length(v);
    if (len > 0.0f) {
        return {v.x / len, v.y / len};
    }
    return v;
}

inline float dot(
    const vec2f& a, const vec2f& b
) {
    return a.x * b.x + a.y * b.y;
}

inline float cross(
    const vec2f& a, const vec2f& b
) {
    return a.x * b.y - a.y * b.x;
}

inline vec2f lerp(
    const vec2f& a, const vec2f& b, float t
) {
    return {a.x + t * (b.x - a.x), a.y + t * (b.y - a.y)};
}

inline vec2f min(
    const vec2f& a, const vec2f& b
) {
    return {std::min(a.x, b.x), std::min(a.y, b.y)};
}

inline vec2f max(
    const vec2f& a, const vec2f& b
) {
    return {std::max(a.x, b.x), std::max(a.y, b.y)};
}

inline vec2f abs(
    const vec2f& v
) {
    return {std::abs(v.x), std::abs(v.y)};
}

inline vec2f floor(
    const vec2f& v
) {
    return {std::floor(v.x), std::floor(v.y)};
}

inline vec2f ceil(
    const vec2f& v
) {
    return {std::ceil(v.x), std::ceil(v.y)};
}

inline vec2f round(
    const vec2f& v
) {
    return {std::round(v.x), std::round(v.y)};
}

inline vec2f fract(
    const vec2f& v
) {
    return {v.x - std::floor(v.x), v.y - std::floor(v.y)};
}

inline vec2f sign(
    const vec2f& v
) {
    return {
        (v.x > 0.0f) ? 1.0f : ((v.x < 0.0f) ? -1.0f : 0.0f),
        (v.y > 0.0f) ? 1.0f : ((v.y < 0.0f) ? -1.0f : 0.0f)
    };
}

inline vec2f clamp(
    const vec2f& v, const vec2f& min_v, const vec2f& max_v
) {
    return {clamp(v.x, min_v.x, max_v.x), clamp(v.y, min_v.y, max_v.y)};
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

inline vec3f min(
    const vec3f& a, const vec3f& b
) {
    return {std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)};
}

inline vec3f max(
    const vec3f& a, const vec3f& b
) {
    return {std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)};
}

inline vec3f abs(
    const vec3f& v
) {
    return {std::abs(v.x), std::abs(v.y), std::abs(v.z)};
}

inline vec3f floor(
    const vec3f& v
) {
    return {std::floor(v.x), std::floor(v.y), std::floor(v.z)};
}

inline vec3f ceil(
    const vec3f& v
) {
    return {std::ceil(v.x), std::ceil(v.y), std::ceil(v.z)};
}

inline vec3f round(
    const vec3f& v
) {
    return {std::round(v.x), std::round(v.y), std::round(v.z)};
}

inline vec3f fract(
    const vec3f& v
) {
    return {v.x - std::floor(v.x), v.y - std::floor(v.y), v.z - std::floor(v.z)};
}

inline vec3f sign(
    const vec3f& v
) {
    return {
        (v.x > 0.0f) ? 1.0f : ((v.x < 0.0f) ? -1.0f : 0.0f),
        (v.y > 0.0f) ? 1.0f : ((v.y < 0.0f) ? -1.0f : 0.0f),
        (v.z > 0.0f) ? 1.0f : ((v.z < 0.0f) ? -1.0f : 0.0f)
    };
}

inline vec3f clamp(
    const vec3f& v, const vec3f& min_v, const vec3f& max_v
) {
    return {
        clamp(v.x, min_v.x, max_v.x), clamp(v.y, min_v.y, max_v.y), clamp(v.z, min_v.z, max_v.z)
    };
}

inline float distance(
    const vec3f& a, const vec3f& b
) {
    return length(b - a);
}

inline float distance_squared(
    const vec3f& a, const vec3f& b
) {
    return length_squared(b - a);
}

inline float angle(
    const vec3f& a, const vec3f& b
) {
    float len_a = length(a);
    float len_b = length(b);
    if (len_a > 0.0f && len_b > 0.0f) {
        float cos_angle = clamp(dot(a, b) / (len_a * len_b), -1.0f, 1.0f);
        return std::acos(cos_angle);
    }
    return 0.0f;
}

inline vec3f reflect(
    const vec3f& incident, const vec3f& normal
) {
    return incident - normal * (2.0f * dot(incident, normal));
}

inline vec3f refract(
    const vec3f& incident, const vec3f& normal, float eta
) {
    float cos_i  = -dot(incident, normal);
    float sin2_t = eta * eta * (1.0f - cos_i * cos_i);
    if (sin2_t > 1.0f) {
        // Полное внутреннее отражение
        return reflect(incident, normal);
    }
    float cos_t = std::sqrt(1.0f - sin2_t);
    return incident * eta + normal * (eta * cos_i - cos_t);
}

inline vec3f project(
    const vec3f& a, const vec3f& onto
) {
    float len_sq = length_squared(onto);
    if (len_sq > 0.0f) {
        return onto * (dot(a, onto) / len_sq);
    }
    return vec3f{0.0f, 0.0f, 0.0f};
}

inline vec3f reject(
    const vec3f& a, const vec3f& from
) {
    return a - project(a, from);
}

inline float length(
    const vec4f& v
) {
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
}

inline float length_squared(
    const vec4f& v
) {
    return v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
}

inline vec4f normalize(
    const vec4f& v
) {
    float len = length(v);
    if (len > 0.0f) {
        return {v.x / len, v.y / len, v.z / len, v.w / len};
    }
    return v;
}

inline float dot(
    const vec4f& a, const vec4f& b
) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

inline vec4f lerp(
    const vec4f& a, const vec4f& b, float t
) {
    return {
        a.x + t * (b.x - a.x), a.y + t * (b.y - a.y), a.z + t * (b.z - a.z), a.w + t * (b.w - a.w)
    };
}

inline vec4f min(
    const vec4f& a, const vec4f& b
) {
    return {std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z), std::min(a.w, b.w)};
}

inline vec4f max(
    const vec4f& a, const vec4f& b
) {
    return {std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z), std::max(a.w, b.w)};
}

inline vec4f abs(
    const vec4f& v
) {
    return {std::abs(v.x), std::abs(v.y), std::abs(v.z), std::abs(v.w)};
}

inline vec4f floor(
    const vec4f& v
) {
    return {std::floor(v.x), std::floor(v.y), std::floor(v.z), std::floor(v.w)};
}

inline vec4f ceil(
    const vec4f& v
) {
    return {std::ceil(v.x), std::ceil(v.y), std::ceil(v.z), std::ceil(v.w)};
}

inline vec4f round(
    const vec4f& v
) {
    return {std::round(v.x), std::round(v.y), std::round(v.z), std::round(v.w)};
}

inline vec4f fract(
    const vec4f& v
) {
    return {
        v.x - std::floor(v.x), v.y - std::floor(v.y), v.z - std::floor(v.z), v.w - std::floor(v.w)
    };
}

inline vec4f sign(
    const vec4f& v
) {
    return {
        (v.x > 0.0f) ? 1.0f : ((v.x < 0.0f) ? -1.0f : 0.0f),
        (v.y > 0.0f) ? 1.0f : ((v.y < 0.0f) ? -1.0f : 0.0f),
        (v.z > 0.0f) ? 1.0f : ((v.z < 0.0f) ? -1.0f : 0.0f),
        (v.w > 0.0f) ? 1.0f : ((v.w < 0.0f) ? -1.0f : 0.0f)
    };
}

inline vec4f clamp(
    const vec4f& v, const vec4f& min_v, const vec4f& max_v
) {
    return {
        clamp(v.x, min_v.x, max_v.x),
        clamp(v.y, min_v.y, max_v.y),
        clamp(v.z, min_v.z, max_v.z),
        clamp(v.w, min_v.w, max_v.w)
    };
}

inline mat4f perspective_matrix(
    float fov, float aspect, float near, float far
) {
    mat4f matrix  = identity_matrix();
    const float f = 1.0f / std::tan(radians(fov * 0.5f));

    matrix[0, 0] = f / aspect;
    matrix[1, 1] = -f;
    matrix[2, 2] = far / (near - far);
    matrix[2, 3] = (near * far) / (near - far);
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
    matrix[0, 3] = -(right + left) / (right - left);
    matrix[1, 3] = -(top + bottom) / (top - bottom);
    matrix[2, 3] = -near / (far - near);

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

inline float lerp(
    float a, float b, float t
) {
    return a + t * (b - a);
}

inline vec3f lerp(
    const vec3f& a, const vec3f& b, float t
) {
    return {a.x + t * (b.x - a.x), a.y + t * (b.y - a.y), a.z + t * (b.z - a.z)};
}

inline transform lerp(
    const transform& a, const transform& b, float t
) {
    transform result;
    result.set_position(lerp(a.get_position(), b.get_position(), t));

    quat qa = euler_to_quat(a.get_rotation());
    quat qb = euler_to_quat(b.get_rotation());
    result.set_rotation(quat_to_euler(slerp(qa, qb, t)));

    result.set_scale(lerp(a.get_scale(), b.get_scale(), t));
    result.set_origin(lerp(a.get_origin(), b.get_origin(), t));
    return result;
}

inline float smoothstep(
    float edge0, float edge1, float x
) {
    float t = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

inline vec2f smoothstep(
    const vec2f& edge0, const vec2f& edge1, const vec2f& x
) {
    return {smoothstep(edge0.x, edge1.x, x.x), smoothstep(edge0.y, edge1.y, x.y)};
}

inline vec3f smoothstep(
    const vec3f& edge0, const vec3f& edge1, const vec3f& x
) {
    return {
        smoothstep(edge0.x, edge1.x, x.x),
        smoothstep(edge0.y, edge1.y, x.y),
        smoothstep(edge0.z, edge1.z, x.z)
    };
}

inline vec4f smoothstep(
    const vec4f& edge0, const vec4f& edge1, const vec4f& x
) {
    return {
        smoothstep(edge0.x, edge1.x, x.x),
        smoothstep(edge0.y, edge1.y, x.y),
        smoothstep(edge0.z, edge1.z, x.z),
        smoothstep(edge0.w, edge1.w, x.w)
    };
}

inline vec3f ease_in(
    const vec3f& a, const vec3f& b, float t
) {
    float eased_t = t * t;
    return lerp(a, b, eased_t);
}

inline vec3f ease_out(
    const vec3f& a, const vec3f& b, float t
) {
    float eased_t = t * (2.0f - t);
    return lerp(a, b, eased_t);
}

inline vec3f ease_in_out(
    const vec3f& a, const vec3f& b, float t
) {
    float eased_t;
    if (t < 0.5f) {
        eased_t = 2.0f * t * t;
    } else {
        eased_t = -1.0f + (4.0f - 2.0f * t) * t;
    }
    return lerp(a, b, eased_t);
}

inline float evaluate_cubic_bezier(
    float t, float p0, float p1, float p2, float p3
) {
    float one_minus_t    = 1.0f - t;
    float one_minus_t_sq = one_minus_t * one_minus_t;
    float one_minus_t_cb = one_minus_t_sq * one_minus_t;
    float t_sq           = t * t;
    float t_cb           = t_sq * t;

    return one_minus_t_cb * p0 + 3.0f * one_minus_t_sq * t * p1 + 3.0f * one_minus_t * t_sq * p2 +
        t_cb * p3;
}

inline uint8 lerp(
    uint8 a, uint8 b, float t
) {
    auto fa = static_cast<float>(a);
    auto fb = static_cast<float>(b);
    return static_cast<uint8>(fa + t * (fb - fa));
}

inline vec3f cubic_bezier(
    const vec3f& a, const vec3f& b, float t, float control1, float control2
) {
    float bezier_t = evaluate_cubic_bezier(t, 0.0f, control1, control2, 1.0f);
    return lerp(a, b, bezier_t);
}

inline float apply_easing(
    float t, interpolation_type type
) {
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
    float t, interpolation_type type, float control1, float control2
) {
    if (type == interpolation_type::cubic_bezier) {
        t = clamp(t, 0.0f, 1.0f);
        return evaluate_cubic_bezier(t, 0.0f, control1, control2, 1.0f);
    }
    return apply_easing(t, type);
}

inline vec3f interpolate(
    const vec3f& a, const vec3f& b, float t, interpolation_type type, float control1, float control2
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
    color a, color b, float t, interpolation_type type, float control1, float control2
) {
    if (type == interpolation_type::step) {
        return {t < 1.0f ? a : b};
    }

    float eased_t = apply_easing_bezier(t, type, control1, control2);

    uint8 r   = lerp(a.r(), b.r(), eased_t);
    uint8 g   = lerp(a.g(), b.g(), eased_t);
    uint8 b_c = lerp(a.b(), b.b(), eased_t);
    uint8 a_c = lerp(a.a(), b.a(), eased_t);

    return {r, g, b_c, a_c};
}

inline float dot(
    const quat& a, const quat& b
) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

inline quat normalize(
    const quat& q
) {
    float len = std::sqrt(dot(q, q));
    if (len > 0.0f) {
        float inv = 1.0f / len;
        return {q.x * inv, q.y * inv, q.z * inv, q.w * inv};
    }
    return q;
}

inline quat slerp(
    const quat& a, const quat& b, float t
) {
    float d = dot(a, b);

    quat b2 = b;
    if (d < 0.0f) {
        b2 = -b;
        d  = -d;
    }

    if (d > 0.9995f) {
        auto result = normalize(
            quat{
                a.x + t * (b2.x - a.x),
                a.y + t * (b2.y - a.y),
                a.z + t * (b2.z - a.z),
                a.w + t * (b2.w - a.w)
            }
        );
        return result;
    }

    float theta_0     = std::acos(d);
    float theta       = theta_0 * t;
    float sin_theta   = std::sin(theta);
    float sin_theta_0 = std::sin(theta_0);

    float s0 = std::cos(theta) - d * sin_theta / sin_theta_0;
    float s1 = sin_theta / sin_theta_0;

    return {a.x * s0 + b2.x * s1, a.y * s0 + b2.y * s1, a.z * s0 + b2.z * s1, a.w * s0 + b2.w * s1};
}

inline quat euler_to_quat(
    const vec3f& euler
) {
    float cx = std::cos(euler.x * 0.5f);
    float sx = std::sin(euler.x * 0.5f);
    float cy = std::cos(euler.y * 0.5f);
    float sy = std::sin(euler.y * 0.5f);
    float cz = std::cos(euler.z * 0.5f);
    float sz = std::sin(euler.z * 0.5f);

    return {
        sx * cy * cz - cx * sy * sz,
        cx * sy * cz + sx * cy * sz,
        cx * cy * sz - sx * sy * cz,
        cx * cy * cz + sx * sy * sz
    };
}

inline vec3f quat_to_euler(
    const quat& q
) {
    float sinr_cosp = 2.0f * (q.w * q.x + q.y * q.z);
    float cosr_cosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
    float rx        = std::atan2(sinr_cosp, cosr_cosp);

    float sinp = 2.0f * (q.w * q.y - q.z * q.x);
    float ry;
    if (std::abs(sinp) >= 1.0f) {
        ry = std::copysign(pi * 0.5f, sinp);
    } else {
        ry = std::asin(sinp);
    }

    float siny_cosp = 2.0f * (q.w * q.z + q.x * q.y);
    float cosy_cosp = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
    float rz        = std::atan2(siny_cosp, cosy_cosp);

    return {rx, ry, rz};
}

inline quat interpolate(
    const quat& a, const quat& b, float t, interpolation_type type, float control1, float control2
) {
    float eased_t = apply_easing_bezier(t, type, control1, control2);
    return slerp(a, b, eased_t);
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
    float a, float epsilon
) {
    return std::abs(a) < epsilon;
}

inline std::expected<mat4f, error_type> inverse_matrix(
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

    if (is_safe_zero(det)) {
        return std::unexpected{error_type::zero_determinant};
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
