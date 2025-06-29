#pragma once

#include "types.h"

namespace voxel {

// Структура для хранения трансформации объекта
struct transform {
private:
    vec3f position_{0.0f, 0.0f, 0.0f};
    vec3f rotation_{0.0f, 0.0f, 0.0f}; // углы в радианах
    vec3f scale_{1.0f, 1.0f, 1.0f};
    
    // Кэшированная матрица трансформации
    mutable mat4f cached_matrix;
    mutable bool matrix_dirty = true;

public:
    // Геттеры
    const vec3f& get_position() const { return position_; }
    const vec3f& get_rotation() const { return rotation_; }
    const vec3f& get_scale() const { return scale_; }
    
    // Получить матрицу трансформации (с кэшированием)
    const mat4f& get_matrix() const;
    
    // Методы для изменения трансформации
    void set_position(const vec3f& pos);
    void set_rotation(const vec3f& rot);
    void set_scale(const vec3f& scl);
    
    void translate(const vec3f& offset);
    void rotate(const vec3f& angles);
    void scale(const vec3f& factor);
    
    // Сбросить кэш матрицы
    void mark_dirty() const { matrix_dirty = true; }
};

} 