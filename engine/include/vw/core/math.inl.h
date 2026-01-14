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
    const vec3f& eye, const vec3f& center, const vec3f& up
) {
    mat4f matrix  = identity_matrix();
    const vec3f f = normalize(center - eye);
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
