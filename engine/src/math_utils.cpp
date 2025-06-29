#include <voxel/math_utils.h>
#include <cstring>

namespace voxel {
namespace math {

mat4f perspective_matrix(float fov, float aspect, float near, float far) {
    mat4f matrix;
    float f = 1.0f / std::tan(radians(fov * 0.5f));
    
    matrix(0, 0) = f / aspect; matrix(0, 1) = 0.0f; matrix(0, 2) = 0.0f; matrix(0, 3) = 0.0f;
    matrix(1, 0) = 0.0f; matrix(1, 1) = f; matrix(1, 2) = 0.0f; matrix(1, 3) = 0.0f;
    matrix(2, 0) = 0.0f; matrix(2, 1) = 0.0f; matrix(2, 2) = (far + near) / (near - far); matrix(2, 3) = -1.0f;
    matrix(3, 0) = 0.0f; matrix(3, 1) = 0.0f; matrix(3, 2) = (2.0f * far * near) / (near - far); matrix(3, 3) = 0.0f;
    
    return matrix;
}

mat4f look_at_matrix(const vec3f& eye, const vec3f& center, const vec3f& up) {
    mat4f matrix;
    vec3f f = normalize(center - eye);
    vec3f s = normalize(cross(f, up));
    vec3f u = cross(s, f);
    
    matrix(0, 0) = s.x; matrix(0, 1) = u.x; matrix(0, 2) = -f.x; matrix(0, 3) = 0.0f;
    matrix(1, 0) = s.y; matrix(1, 1) = u.y; matrix(1, 2) = -f.y; matrix(1, 3) = 0.0f;
    matrix(2, 0) = s.z; matrix(2, 1) = u.z; matrix(2, 2) = -f.z; matrix(2, 3) = 0.0f;
    matrix(3, 0) = -dot(s, eye); matrix(3, 1) = -dot(u, eye); matrix(3, 2) = dot(f, eye); matrix(3, 3) = 1.0f;
    
    return matrix;
}

mat4f multiply_matrices(const mat4f& a, const mat4f& b) {
    mat4f result;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result(i, j) = 0.0f;
            for (int k = 0; k < 4; k++) {
                result(i, j) += a(i, k) * b(k, j);
            }
        }
    }
    return result;
}

// Матрицы трансформации
mat4f identity_matrix() {
    mat4f matrix;
    matrix(0, 0) = 1.0f; matrix(0, 1) = 0.0f; matrix(0, 2) = 0.0f; matrix(0, 3) = 0.0f;
    matrix(1, 0) = 0.0f; matrix(1, 1) = 1.0f; matrix(1, 2) = 0.0f; matrix(1, 3) = 0.0f;
    matrix(2, 0) = 0.0f; matrix(2, 1) = 0.0f; matrix(2, 2) = 1.0f; matrix(2, 3) = 0.0f;
    matrix(3, 0) = 0.0f; matrix(3, 1) = 0.0f; matrix(3, 2) = 0.0f; matrix(3, 3) = 1.0f;
    return matrix;
}

mat4f translation_matrix(const vec3f& translation) {
    mat4f matrix = identity_matrix();
    matrix(3, 0) = translation.x;
    matrix(3, 1) = translation.y;
    matrix(3, 2) = translation.z;
    return matrix;
}

mat4f rotation_matrix_x(float angle) {
    mat4f matrix = identity_matrix();
    float c = std::cos(angle);
    float s = std::sin(angle);
    
    matrix(1, 1) = c;  matrix(1, 2) = -s;
    matrix(2, 1) = s;  matrix(2, 2) = c;
    
    return matrix;
}

mat4f rotation_matrix_y(float angle) {
    mat4f matrix = identity_matrix();
    float c = std::cos(angle);
    float s = std::sin(angle);
    
    matrix(0, 0) = c;  matrix(0, 2) = s;
    matrix(2, 0) = -s; matrix(2, 2) = c;
    
    return matrix;
}

mat4f rotation_matrix_z(float angle) {
    mat4f matrix = identity_matrix();
    float c = std::cos(angle);
    float s = std::sin(angle);
    
    matrix(0, 0) = c;  matrix(0, 1) = -s;
    matrix(1, 0) = s;  matrix(1, 1) = c;
    
    return matrix;
}

mat4f rotation_matrix(const vec3f& rotation) {
    // Комбинированная матрица поворота: Z * Y * X
    mat4f rot_x = rotation_matrix_x(rotation.x);
    mat4f rot_y = rotation_matrix_y(rotation.y);
    mat4f rot_z = rotation_matrix_z(rotation.z);
    
    return multiply_matrices(multiply_matrices(rot_z, rot_y), rot_x);
}

mat4f scale_matrix(const vec3f& scale) {
    mat4f matrix = identity_matrix();
    matrix(0, 0) = scale.x;
    matrix(1, 1) = scale.y;
    matrix(2, 2) = scale.z;
    return matrix;
}

mat4f transform_matrix(const vec3f& position, const vec3f& rotation, const vec3f& scale) {
    // Комбинированная матрица трансформации: T * R * S
    mat4f trans = translation_matrix(position);
    mat4f rot = rotation_matrix(rotation);
    mat4f scl = scale_matrix(scale);
    
    return multiply_matrices(multiply_matrices(trans, rot), scl);
}

// Утилиты для матриц
mat4f transpose_matrix(const mat4f& matrix) {
    mat4f result;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result(i, j) = matrix(j, i);
        }
    }
    return result;
}

mat4f inverse_matrix(const mat4f& matrix) {
    // Простая реализация обратной матрицы для ортогональных матриц
    // Для полной реализации нужен более сложный алгоритм (например, LU разложение)
    mat4f result = transpose_matrix(matrix);
    
    // Для матриц трансформации (T*R*S) обратная матрица = S^(-1) * R^T * T^(-1)
    // Это упрощенная версия, работает только для определенных случаев
    return result;
}

} // namespace math
} // namespace voxel 