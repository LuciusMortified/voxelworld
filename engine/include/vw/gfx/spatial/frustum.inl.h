#pragma once

#ifndef VW_GFX_SPATIAL_FRUSTUM_INL_H
#define VW_GFX_SPATIAL_FRUSTUM_INL_H

#include <algorithm>
#include <cmath>
#include <limits>

#include "vw/core/math.h"
#include "vw/gfx/spatial/aabb.h"
#include "vw/gfx/spatial/frustum.h"
#include "vw/gfx/spatial/ray.h"

namespace vw::gfx {

inline frustum frustum::from_view_projection_matrix(
    const mat4f& view_proj
) {
    frustum f;

    // Извлечь плоскости напрямую из view-projection матрицы (column-major)
    // Плоскости хранятся в виде: ax + by + cz + d = 0
    // Где (a, b, c) - нормаль, d - расстояние
    // Алгоритм: для column-major матрицы извлекаем строки и комбинируем их

    // Left plane: row3 + row0 (Row4 + Row1 в 1-indexed)
    {
        vec4f plane_vec{
            view_proj[3, 0] + view_proj[0, 0],
            view_proj[3, 1] + view_proj[0, 1],
            view_proj[3, 2] + view_proj[0, 2],
            view_proj[3, 3] + view_proj[0, 3]
        };
        float length = vw::math::length(vec3f{plane_vec.x, plane_vec.y, plane_vec.z});
        if (length > 0.0f) {
            float inv_length     = 1.0f / length;
            f.planes[0].normal   = vec3f{plane_vec.x, plane_vec.y, plane_vec.z} * inv_length;
            f.planes[0].distance = plane_vec.w * inv_length;
        }
    }

    // Right plane: row3 - row0 (Row4 - Row1 в 1-indexed)
    {
        vec4f plane_vec{
            view_proj[3, 0] - view_proj[0, 0],
            view_proj[3, 1] - view_proj[0, 1],
            view_proj[3, 2] - view_proj[0, 2],
            view_proj[3, 3] - view_proj[0, 3]
        };
        float length = vw::math::length(vec3f{plane_vec.x, plane_vec.y, plane_vec.z});
        if (length > 0.0f) {
            float inv_length     = 1.0f / length;
            f.planes[1].normal   = vec3f{plane_vec.x, plane_vec.y, plane_vec.z} * inv_length;
            f.planes[1].distance = plane_vec.w * inv_length;
        }
    }

    // Bottom plane: row3 + row1 (Row4 + Row2 в 1-indexed)
    {
        vec4f plane_vec{
            view_proj[3, 0] + view_proj[1, 0],
            view_proj[3, 1] + view_proj[1, 1],
            view_proj[3, 2] + view_proj[1, 2],
            view_proj[3, 3] + view_proj[1, 3]
        };
        float length = vw::math::length(vec3f{plane_vec.x, plane_vec.y, plane_vec.z});
        if (length > 0.0f) {
            float inv_length     = 1.0f / length;
            f.planes[2].normal   = vec3f{plane_vec.x, plane_vec.y, plane_vec.z} * inv_length;
            f.planes[2].distance = plane_vec.w * inv_length;
        }
    }

    // Top plane: row3 - row1 (Row4 - Row2 в 1-indexed)
    {
        vec4f plane_vec{
            view_proj[3, 0] - view_proj[1, 0],
            view_proj[3, 1] - view_proj[1, 1],
            view_proj[3, 2] - view_proj[1, 2],
            view_proj[3, 3] - view_proj[1, 3]
        };
        float length = vw::math::length(vec3f{plane_vec.x, plane_vec.y, plane_vec.z});
        if (length > 0.0f) {
            float inv_length     = 1.0f / length;
            f.planes[3].normal   = vec3f{plane_vec.x, plane_vec.y, plane_vec.z} * inv_length;
            f.planes[3].distance = plane_vec.w * inv_length;
        }
    }

    // Near plane: row3 + row2 (Row4 + Row3 в 1-indexed)
    {
        vec4f plane_vec{
            view_proj[3, 0] + view_proj[2, 0],
            view_proj[3, 1] + view_proj[2, 1],
            view_proj[3, 2] + view_proj[2, 2],
            view_proj[3, 3] + view_proj[2, 3]
        };
        float length = vw::math::length(vec3f{plane_vec.x, plane_vec.y, plane_vec.z});
        if (length > 0.0f) {
            float inv_length     = 1.0f / length;
            f.planes[4].normal   = vec3f{plane_vec.x, plane_vec.y, plane_vec.z} * inv_length;
            f.planes[4].distance = plane_vec.w * inv_length;
        }
    }

    // Far plane: row3 - row2 (Row4 - Row3 в 1-indexed)
    {
        vec4f plane_vec{
            view_proj[3, 0] - view_proj[2, 0],
            view_proj[3, 1] - view_proj[2, 1],
            view_proj[3, 2] - view_proj[2, 2],
            view_proj[3, 3] - view_proj[2, 3]
        };
        float length = vw::math::length(vec3f{plane_vec.x, plane_vec.y, plane_vec.z});
        if (length > 0.0f) {
            float inv_length     = 1.0f / length;
            f.planes[5].normal   = vec3f{plane_vec.x, plane_vec.y, plane_vec.z} * inv_length;
            f.planes[5].distance = plane_vec.w * inv_length;
        }
    }

    return f;
}

inline bool frustum::intersects(
    const vec3f& point
) const {
    for (int i = 0; i < 6; ++i) {
        float dist = vw::math::dot(planes[i].normal, point) + planes[i].distance;
        if (dist < 0.0f) {
            return false;
        }
    }
    return true;
}

inline bool frustum::intersects(
    const aabb& bounds
) const {
    for (int i = 0; i < 6; ++i) {
        // Вычислить положительную вершину AABB (p-vertex) для плоскости
        const vec3f p_vertex{
            planes[i].normal.x > 0.0f ? bounds.max.x : bounds.min.x,
            planes[i].normal.y > 0.0f ? bounds.max.y : bounds.min.y,
            planes[i].normal.z > 0.0f ? bounds.max.z : bounds.min.z
        };

        // Если положительная вершина находится вне плоскости, AABB не пересекается
        const float dist = vw::math::dot(planes[i].normal, p_vertex) + planes[i].distance;
        if (dist < 0.0f) {
            return false;
        }
    }
    return true;
}

inline bool frustum::intersects(
    const ray& r
) const {
    // Найти точки пересечения луча с каждой плоскостью
    float t_min = 0.0f;
    float t_max = r.length();

    for (int i = 0; i < 6; ++i) {
        const plane& p = planes[i];

        // Вычислить скалярное произведение направления луча и нормали плоскости
        float denom = vw::math::dot(p.normal, r.direction);

        // Если луч параллелен плоскости, проверить, находится ли начальная точка внутри
        if (std::abs(denom) < 1e-6f) {
            float dist = vw::math::dot(p.normal, r.start) + p.distance;
            if (dist < 0.0f) {
                return false;  // Луч полностью вне frustum
            }
            continue;
        }

        // Вычислить параметр t точки пересечения
        float t = -(vw::math::dot(p.normal, r.start) + p.distance) / denom;

        // Обновить диапазон параметров
        if (denom > 0.0f) {
            // Луч входит в полупространство
            t_max = std::min(t_max, t);
        } else {
            // Луч выходит из полупространства
            t_min = std::max(t_min, t);
        }

        // Если диапазон стал пустым, луч не пересекает frustum
        if (t_min > t_max) {
            return false;
        }
    }

    // Проверить, что диапазон пересечений находится в пределах длины луча
    return t_min <= r.length() && t_max >= 0.0f;
}

inline bool frustum::operator==(
    const frustum& other
) const {
    for (int i = 0; i < 6; ++i) {
        if (planes[i].normal != other.planes[i].normal ||
            std::abs(planes[i].distance - other.planes[i].distance) > 1e-5f) {
            return false;
        }
    }
    return true;
}

inline bool frustum::operator!=(
    const frustum& other
) const {
    return !(*this == other);
}

inline bool frustum::approximately_equal(
    const frustum& other, float angle_threshold, float distance_threshold
) const {
    for (int i = 0; i < 6; ++i) {
        const float angle_diff = std::acos(
            math::clamp(math::dot(planes[i].normal, other.planes[i].normal), -1.0f, 1.0f)
        );
        const float distance_diff = std::abs(planes[i].distance - other.planes[i].distance);

        if (angle_diff > angle_threshold || distance_diff > distance_threshold) {
            return false;
        }
    }
    return true;
}

}  // namespace vw::gfx

#endif  // VW_GFX_SPATIAL_FRUSTUM_INL_H
