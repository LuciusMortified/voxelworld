#include "vw/core/math.h"

namespace vw::math {

mat4f perspective_matrix(
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

mat4f look_at_matrix(
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

mat4f identity_matrix() {
    mat4f matrix;
    matrix[0, 0] = 1.0f;
    matrix[1, 1] = 1.0f;
    matrix[2, 2] = 1.0f;
    matrix[3, 3] = 1.0f;
    return matrix;
}

mat4f translation_matrix(
    const vec3f& translation
) {
    mat4f matrix = identity_matrix();
    matrix[0, 3] = translation.x;
    matrix[1, 3] = translation.y;
    matrix[2, 3] = translation.z;
    return matrix;
}

mat4f rotation_matrix_x(
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

mat4f rotation_matrix_y(
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

mat4f rotation_matrix_z(
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

mat4f rotation_matrix(
    const vec3f& rotation
) {
    const mat4f rot_x = rotation_matrix_x(rotation.x);
    const mat4f rot_y = rotation_matrix_y(rotation.y);
    const mat4f rot_z = rotation_matrix_z(rotation.z);

    return rot_z * rot_y * rot_x;
}

mat4f scale_matrix(
    const vec3f& scale
) {
    mat4f matrix = identity_matrix();
    matrix[0, 0] = scale.x;
    matrix[1, 1] = scale.y;
    matrix[2, 2] = scale.z;
    return matrix;
}

mat4f transform_matrix(
    const vec3f& position, const vec3f& rotation, const vec3f& scale, const vec3f& origin
) {
    const mat4f trans        = translation_matrix(position);
    const mat4f orig_back    = translation_matrix(origin);
    const mat4f orig_forward = translation_matrix(-origin);
    const mat4f rot          = rotation_matrix(rotation);
    const mat4f scl          = scale_matrix(scale);

    return trans * orig_back * rot * scl * orig_forward;
}

// Утилиты для матриц
mat4f transpose_matrix(
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

// TODO: Реализовать обратную матрицу
mat4f inverse_matrix(
    const mat4f& matrix
) {
    // Простая реализация обратной матрицы для ортогональных матриц
    // Для полной реализации нужен более сложный алгоритм (например, LU разложение)
    mat4f result = transpose_matrix(matrix);

    // Для матриц трансформации (T*R*S) обратная матрица = S^(-1) * R^T * T^(-1)
    // Это упрощенная версия, работает только для определенных случаев
    return result;
}

}  // namespace vw::math
