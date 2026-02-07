#pragma once

#ifndef VW_CORE_MATH_H
#define VW_CORE_MATH_H

#include <cmath>

#include "vw/core/mat4.h"
#include "vw/core/vec3.h"

namespace vw::math {

enum class interpolation_type : uint8 {
    linear,
    step,
    ease_in,
    ease_out,
    ease_in_out,
    cubic_bezier
};

constexpr float pi         = 3.14159265359f;
constexpr float deg_to_rad = pi / 180.0f;
constexpr float rad_to_deg = 180.0f / pi;

float radians(float degrees);
float degrees(float radians);

float length(const vec3f& v);
float length_squared(const vec3f& v);
vec3f normalize(const vec3f& v);
vec3f cross(const vec3f& a, const vec3f& b);
float dot(const vec3f& a, const vec3f& b);
float clamp(float value, float min_val, float max_val);
float lerp(float a, float b, float t);
vec3f lerp(const vec3f& a, const vec3f& b, float t);

vec3f ease_in(const vec3f& a, const vec3f& b, float t);
vec3f ease_out(const vec3f& a, const vec3f& b, float t);
vec3f ease_in_out(const vec3f& a, const vec3f& b, float t);
float cubic_bezier(float t, float p0, float p1, float p2, float p3);
vec3f cubic_bezier(const vec3f& a, const vec3f& b, float t, float control1, float control2);
float apply_easing(float t, interpolation_type type);
float apply_easing_bezier(float t, interpolation_type type, float control1, float control2);
vec3f interpolate(const vec3f& a, const vec3f& b, float t, interpolation_type type, float control1 = 0.0f, float control2 = 1.0f);

vec3f perpendicular(const vec3f& eye, const vec3f& target);

bool is_safe_zero(float a, float b, float epsilon = 1e-5f);

mat4f perspective_matrix(float fov, float aspect, float near, float far);
mat4f orthographic_matrix(float left, float right, float bottom, float top, float near, float far);
mat4f look_at_matrix(const vec3f& eye, const vec3f& target);
mat4f look_at_matrix(const vec3f& eye, const vec3f& target, const vec3f& up);

mat4f translation_matrix(const vec3f& translation);
mat4f rotation_matrix_x(float angle);
mat4f rotation_matrix_y(float angle);
mat4f rotation_matrix_z(float angle);
mat4f rotation_matrix(const vec3f& rotation);  // комбинированная матрица поворота ZYX
mat4f scale_matrix(const vec3f& scale);
mat4f transform_matrix(
    const vec3f& position, const vec3f& rotation, const vec3f& scale, const vec3f& origin
);

mat4f identity_matrix();
mat4f transpose_matrix(const mat4f& matrix);
mat4f inverse_matrix(const mat4f& matrix);

}  // namespace vw::math

#include "vw/core/math.inl.h"

#endif  // VW_CORE_MATH_H
