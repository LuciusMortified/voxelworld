#pragma once

#ifndef VW_GFX_SPATIAL_RAY_H
#define VW_GFX_SPATIAL_RAY_H

#include "vw/core.h"

namespace vw::gfx {

struct aabb;

struct ray {
    vec3f start;      // Начальная точка луча
    vec3f end;        // Конечная точка луча
    vec3f direction;  // нормализованный вектор направления
    
    // Конструктор по двум точкам
    ray(const vec3f& start, const vec3f& end);
    
    [[nodiscard]] float length() const;            // Вычислить длину луча
    [[nodiscard]] vec3f point_at(float t) const;   // t в диапазоне [0, length()]
    [[nodiscard]] bool intersects(const aabb& bounds, float* t_out = nullptr) const;
};

}  // namespace vw::gfx

#include "vw/gfx/spatial/ray.inl.h"

#endif  // VW_GFX_SPATIAL_RAY_H

