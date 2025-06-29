#pragma once
#include <cmath>

#include <voxel/types.h>

namespace voxel {
namespace math {
    // Константы
    constexpr float PI = 3.14159265359f;
    constexpr float DEG_TO_RAD = PI / 180.0f;
    constexpr float RAD_TO_DEG = 180.0f / PI;
    
    // Утилиты для углов
    inline float radians(float degrees) {
        return degrees * DEG_TO_RAD;
    }
    
    inline float degrees(float radians) {
        return radians * RAD_TO_DEG;
    }
    
    // Функции для векторов
    inline float length(const vec3f& v) {
        return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    }
    
    inline float length_squared(const vec3f& v) {
        return v.x * v.x + v.y * v.y + v.z * v.z;
    }
    
    inline vec3f normalize(const vec3f& v) {
        float len = length(v);
        if (len > 0.0f) {
            return vec3f(v.x / len, v.y / len, v.z / len);
        }
        return v;
    }
    
    inline vec3f cross(const vec3f& a, const vec3f& b) {
        return vec3f(
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        );
    }
    
    inline float dot(const vec3f& a, const vec3f& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }
    
    // Функции для матриц 4x4
    mat4f perspective_matrix(float fov, float aspect, float near, float far);
    mat4f look_at_matrix(const vec3f& eye, const vec3f& center, const vec3f& up);
    mat4f multiply_matrices(const mat4f& a, const mat4f& b);
    
    // Матрицы трансформации
    mat4f translation_matrix(const vec3f& translation);
    mat4f rotation_matrix_x(float angle);
    mat4f rotation_matrix_y(float angle);
    mat4f rotation_matrix_z(float angle);
    mat4f rotation_matrix(const vec3f& rotation); // комбинированная матрица поворота
    mat4f scale_matrix(const vec3f& scale);
    mat4f transform_matrix(const vec3f& position, const vec3f& rotation, const vec3f& scale);
    
    // Утилиты для матриц
    mat4f identity_matrix();
    mat4f transpose_matrix(const mat4f& matrix);
    mat4f inverse_matrix(const mat4f& matrix);
    
    // Дополнительные математические функции
    inline float clamp(float value, float min_val, float max_val) {
        if (value < min_val) return min_val;
        if (value > max_val) return max_val;
        return value;
    }
    
    inline float lerp(float a, float b, float t) {
        return a + t * (b - a);
    }
    
    inline vec3f lerp(const vec3f& a, const vec3f& b, float t) {
        return vec3f(
            lerp(a.x, b.x, t),
            lerp(a.y, b.y, t),
            lerp(a.z, b.z, t)
        );
    }
}
} 