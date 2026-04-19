#pragma once

#ifndef VW_SPATIAL_AABB_INL_H
#define VW_SPATIAL_AABB_INL_H

#include <algorithm>
#include <cmath>

#include "vw/core/math.h"
#include "vw/spatial/aabb.h"
#include "vw/spatial/ray.h"

namespace vw::spatial {

inline vec3f aabb::center() const {
    return {
        (min.x + max.x) * 0.5f,
        (min.y + max.y) * 0.5f,
        (min.z + max.z) * 0.5f
    };
}

inline vec3f aabb::size() const {
    return {
        max.x - min.x,
        max.y - min.y,
        max.z - min.z
    };
}

inline float aabb::area() const {
    const vec3f s = size();
    return s.x * s.y + s.y * s.z + s.z * s.x;
}

inline bool aabb::intersects(const aabb& other) const {
    return min.x <= other.max.x && max.x >= other.min.x &&
           min.y <= other.max.y && max.y >= other.min.y &&
           min.z <= other.max.z && max.z >= other.min.z;
}

inline bool aabb::intersects(const vec3f& point) const {
    return point.x >= min.x && point.x <= max.x &&
           point.y >= min.y && point.y <= max.y &&
           point.z >= min.z && point.z <= max.z;
}

inline bool aabb::intersects(const ray& r) const {
    float ray_len = r.length();
    float t_min = 0.0f;
    float t_max = ray_len;

    float inv_dir_x = 1.0f / r.direction.x;
    float t0_x = (min.x - r.start.x) * inv_dir_x;
    float t1_x = (max.x - r.start.x) * inv_dir_x;
    if (inv_dir_x < 0.0f) {
        std::swap(t0_x, t1_x);
    }
    t_min = t0_x > t_min ? t0_x : t_min;
    t_max = t1_x < t_max ? t1_x : t_max;
    if (t_max < t_min) return false;

    float inv_dir_y = 1.0f / r.direction.y;
    float t0_y = (min.y - r.start.y) * inv_dir_y;
    float t1_y = (max.y - r.start.y) * inv_dir_y;
    if (inv_dir_y < 0.0f) {
        std::swap(t0_y, t1_y);
    }
    t_min = t0_y > t_min ? t0_y : t_min;
    t_max = t1_y < t_max ? t1_y : t_max;
    if (t_max < t_min) return false;

    float inv_dir_z = 1.0f / r.direction.z;
    float t0_z = (min.z - r.start.z) * inv_dir_z;
    float t1_z = (max.z - r.start.z) * inv_dir_z;
    if (inv_dir_z < 0.0f) {
        std::swap(t0_z, t1_z);
    }
    t_min = t0_z > t_min ? t0_z : t_min;
    t_max = t1_z < t_max ? t1_z : t_max;
    if (t_max < t_min) return false;

    return true;
}

inline aabb aabb::merge(const aabb& a, const aabb& b) {
    return aabb{
        vec3f{
            std::min(a.min.x, b.min.x),
            std::min(a.min.y, b.min.y),
            std::min(a.min.z, b.min.z)
        },
        vec3f{
            std::max(a.max.x, b.max.x),
            std::max(a.max.y, b.max.y),
            std::max(a.max.z, b.max.z)
        }
    };
}

inline bool aabb::operator==(const aabb& other) const {
    constexpr float epsilon = 1e-5f;
    return math::is_safe_zero(min.x - other.min.x, epsilon) &&
           math::is_safe_zero(min.y - other.min.y, epsilon) &&
           math::is_safe_zero(min.z - other.min.z, epsilon) &&
           math::is_safe_zero(max.x - other.max.x, epsilon) &&
           math::is_safe_zero(max.y - other.max.y, epsilon) &&
           math::is_safe_zero(max.z - other.max.z, epsilon);
}

inline bool aabb::operator!=(const aabb& other) const {
    return !(*this == other);
}

}  // namespace vw::spatial

#endif  // VW_SPATIAL_AABB_INL_H
