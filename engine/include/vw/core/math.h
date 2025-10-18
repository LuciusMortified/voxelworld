#pragma once

#ifndef VW_CORE_MATH_H
#define VW_CORE_MATH_H

#include <cmath>

#include "vw/core/mat4.h"
#include "vw/core/vec3.h"

namespace vw::math {
constexpr float PI         = 3.14159265359f;
constexpr float DEG_TO_RAD = PI / 180.0f;
constexpr float RAD_TO_DEG = 180.0f / PI;

inline float radians(float degrees) {
    return degrees * DEG_TO_RAD;
}

inline float degrees(float radians) {
    return radians * RAD_TO_DEG;
}

inline float length(const vec3f& v) {
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

inline float length_squared(const vec3f& v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

inline vec3f normalize(const vec3f& v) {
    float len = length(v);
    if (len > 0.0f) {
        return {v.x / len, v.y / len, v.z / len};
    }
    return v;
}

inline vec3f cross(const vec3f& a, const vec3f& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

inline float dot(const vec3f& a, const vec3f& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

mat4f perspective_matrix(float fov, float aspect, float near, float far);
mat4f look_at_matrix(const vec3f& eye, const vec3f& center, const vec3f& up);

mat4f translation_matrix(const vec3f& translation);
mat4f rotation_matrix_x(float angle);
mat4f rotation_matrix_y(float angle);
mat4f rotation_matrix_z(float angle);
mat4f rotation_matrix(const vec3f& rotation);  // комбинированная матрица поворота ZYX
mat4f scale_matrix(const vec3f& scale);
mat4f transform_matrix(
    const vec3f& position,
    const vec3f& rotation,
    const vec3f& scale,
    const vec3f& origin
);

mat4f identity_matrix();
mat4f transpose_matrix(const mat4f& matrix);
mat4f inverse_matrix(const mat4f& matrix);

inline float clamp(float value, float min_val, float max_val) {
    if (value < min_val)
        return min_val;
    if (value > max_val)
        return max_val;
    return value;
}

inline float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

inline vec3f lerp(const vec3f& a, const vec3f& b, float t) {
    return {lerp(a.x, b.x, t), lerp(a.y, b.y, t), lerp(a.z, b.z, t)};
}
}  // namespace vw::math

#endif  // VW_CORE_MATH_H
