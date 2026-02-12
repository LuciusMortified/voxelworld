#pragma once

#ifndef VW_CORE_MATH_H
#define VW_CORE_MATH_H

#include <expected>

#include "vw/core/color.h"
#include "vw/core/mat4.h"
#include "vw/core/quat.h"
#include "vw/core/vec2.h"
#include "vw/core/vec3.h"
#include "vw/core/vec4.h"

namespace vw {

struct transform;

}

namespace vw::math {

enum class error_type {
    zero_determinant,
};

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

constexpr float radians(float degrees);
constexpr float degrees(float radians);

template <typename T>
constexpr T clamp(T value, T min_val, T max_val);

template <typename T>
constexpr T sign(T value);

float length(const vec2f& v);
float length_squared(const vec2f& v);
vec2f normalize(const vec2f& v);
float dot(const vec2f& a, const vec2f& b);
float cross(const vec2f& a, const vec2f& b);
vec2f lerp(const vec2f& a, const vec2f& b, float t);

vec2f min(const vec2f& a, const vec2f& b);
vec2f max(const vec2f& a, const vec2f& b);
vec2f abs(const vec2f& v);
vec2f floor(const vec2f& v);
vec2f ceil(const vec2f& v);
vec2f round(const vec2f& v);
vec2f fract(const vec2f& v);
vec2f sign(const vec2f& v);
vec2f clamp(const vec2f& v, const vec2f& min_v, const vec2f& max_v);

float length(const vec3f& v);
float length_squared(const vec3f& v);
vec3f normalize(const vec3f& v);
vec3f cross(const vec3f& a, const vec3f& b);
float dot(const vec3f& a, const vec3f& b);
vec3f lerp(const vec3f& a, const vec3f& b, float t);

vec3f min(const vec3f& a, const vec3f& b);
vec3f max(const vec3f& a, const vec3f& b);
vec3f abs(const vec3f& v);
vec3f floor(const vec3f& v);
vec3f ceil(const vec3f& v);
vec3f round(const vec3f& v);
vec3f fract(const vec3f& v);
vec3f sign(const vec3f& v);
vec3f clamp(const vec3f& v, const vec3f& min_v, const vec3f& max_v);

float distance(const vec3f& a, const vec3f& b);
float distance_squared(const vec3f& a, const vec3f& b);
float angle(const vec3f& a, const vec3f& b);
vec3f reflect(const vec3f& incident, const vec3f& normal);
vec3f refract(const vec3f& incident, const vec3f& normal, float eta);
vec3f project(const vec3f& a, const vec3f& onto);
vec3f reject(const vec3f& a, const vec3f& from);

float length(const vec4f& v);
float length_squared(const vec4f& v);
vec4f normalize(const vec4f& v);
float dot(const vec4f& a, const vec4f& b);
vec4f lerp(const vec4f& a, const vec4f& b, float t);

vec4f min(const vec4f& a, const vec4f& b);
vec4f max(const vec4f& a, const vec4f& b);
vec4f abs(const vec4f& v);
vec4f floor(const vec4f& v);
vec4f ceil(const vec4f& v);
vec4f round(const vec4f& v);
vec4f fract(const vec4f& v);
vec4f sign(const vec4f& v);
vec4f clamp(const vec4f& v, const vec4f& min_v, const vec4f& max_v);

bool approx_equal(float a, float b, float epsilon = 1e-5f);
bool approx_equal(const vec2f& a, const vec2f& b, float epsilon = 1e-5f);
bool approx_equal(const vec3f& a, const vec3f& b, float epsilon = 1e-5f);
bool approx_equal(const vec4f& a, const vec4f& b, float epsilon = 1e-5f);
bool approx_equal(const mat4f& a, const mat4f& b, float epsilon = 1e-5f);
bool approx_equal(const quat& a, const quat& b, float epsilon = 1e-5f);

float lerp(float a, float b, float t);
transform lerp(const transform& a, const transform& b, float t);

vec3f ease_in(const vec3f& a, const vec3f& b, float t);
vec3f ease_out(const vec3f& a, const vec3f& b, float t);
vec3f ease_in_out(const vec3f& a, const vec3f& b, float t);
float evaluate_cubic_bezier(float t, float p0, float p1, float p2, float p3);
vec3f cubic_bezier(const vec3f& a, const vec3f& b, float t, float control1, float control2);
float apply_easing(float t, interpolation_type type);
float apply_easing_bezier(float t, interpolation_type type, float control1, float control2);
uint8 lerp(uint8 a, uint8 b, float t);
vec3f interpolate(
    const vec3f& a,
    const vec3f& b,
    float t,
    interpolation_type type,
    float control1 = 0.0f,
    float control2 = 1.0f
);
color interpolate(
    color a, color b, float t, interpolation_type type, float control1 = 0.0f, float control2 = 1.0f
);

float dot(const quat& a, const quat& b);
quat normalize(const quat& q);
quat slerp(const quat& a, const quat& b, float t);
quat euler_to_quat(const vec3f& euler);
vec3f quat_to_euler(const quat& q);
quat interpolate(
    const quat& a,
    const quat& b,
    float t,
    interpolation_type type,
    float control1 = 0.0f,
    float control2 = 1.0f
);

vec3f perpendicular(const vec3f& eye, const vec3f& target);
bool is_safe_zero(float a, float epsilon = 1e-5f);

float smoothstep(float edge0, float edge1, float x);
vec2f smoothstep(const vec2f& edge0, const vec2f& edge1, const vec2f& x);
vec3f smoothstep(const vec3f& edge0, const vec3f& edge1, const vec3f& x);
vec4f smoothstep(const vec4f& edge0, const vec4f& edge1, const vec4f& x);

inline float mix(float a, float b, float t) { return lerp(a, b, t); }
inline vec2f mix(const vec2f& a, const vec2f& b, float t) { return lerp(a, b, t); }
inline vec3f mix(const vec3f& a, const vec3f& b, float t) { return lerp(a, b, t); }
inline vec4f mix(const vec4f& a, const vec4f& b, float t) { return lerp(a, b, t); }

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
std::expected<mat4f, error_type> inverse_matrix(const mat4f& matrix);

}  // namespace vw::math

#include "vw/core/math.inl.h"

#endif  // VW_CORE_MATH_H
