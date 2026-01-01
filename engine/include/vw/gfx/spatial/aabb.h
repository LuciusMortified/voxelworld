#pragma once

#ifndef VW_GFX_SPATIAL_AABB_H
#define VW_GFX_SPATIAL_AABB_H

#include "vw/core.h"

namespace vw::gfx {

struct ray;

struct aabb {
    vec3f min;
    vec3f max;
    
    // Методы без аллокаций
    [[nodiscard]] vec3f center() const;
    [[nodiscard]] vec3f size() const;
    [[nodiscard]] float area() const;  // Площадь поверхности AABB
    [[nodiscard]] bool intersects(const aabb& other) const;  // Пересечение с другим AABB
    [[nodiscard]] bool intersects(const ray& r) const;       // Пересечение с лучом
    [[nodiscard]] bool intersects(const vec3f& point) const; // Содержит точку
    
    // Статическая функция для объединения двух AABB
    [[nodiscard]] static aabb merge(const aabb& a, const aabb& b);
    
    // Операторы сравнения
    [[nodiscard]] bool operator==(const aabb& other) const;
    [[nodiscard]] bool operator!=(const aabb& other) const;
};

}  // namespace vw::gfx

#include "vw/gfx/spatial/aabb.inl.h"

#endif  // VW_GFX_SPATIAL_AABB_H

