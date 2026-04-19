#pragma once

#ifndef VW_SPATIAL_RAY_INL_H
#define VW_SPATIAL_RAY_INL_H

#include <algorithm>

#include "vw/core/math.h"
#include "vw/spatial/aabb.h"
#include "vw/spatial/ray.h"

namespace vw::spatial {

inline ray::ray(const vec3f& start, const vec3f& end)
    : start(start), end(end) {
    vec3f dir = end - start;
    float len = vw::math::length(dir);
    if (len > 0.0f) {
        direction = vw::math::normalize(dir);
    } else {
        direction = vec3f{1.0f, 0.0f, 0.0f};
    }
}

inline float ray::length() const {
    return vw::math::length(end - start);
}

inline vec3f ray::point_at(float t) const {
    return start + direction * t;
}

inline bool ray::intersects_at(const aabb& bounds, float& t_out) const {
    float ray_len = length();
    float t_min = 0.0f;
    float t_max = ray_len;

    float inv_dir_x = 1.0f / direction.x;
    float t0_x = (bounds.min.x - start.x) * inv_dir_x;
    float t1_x = (bounds.max.x - start.x) * inv_dir_x;
    if (inv_dir_x < 0.0f) {
        std::swap(t0_x, t1_x);
    }
    t_min = t0_x > t_min ? t0_x : t_min;
    t_max = t1_x < t_max ? t1_x : t_max;
    if (t_max < t_min) return false;

    float inv_dir_y = 1.0f / direction.y;
    float t0_y = (bounds.min.y - start.y) * inv_dir_y;
    float t1_y = (bounds.max.y - start.y) * inv_dir_y;
    if (inv_dir_y < 0.0f) {
        std::swap(t0_y, t1_y);
    }
    t_min = t0_y > t_min ? t0_y : t_min;
    t_max = t1_y < t_max ? t1_y : t_max;
    if (t_max < t_min) return false;

    float inv_dir_z = 1.0f / direction.z;
    float t0_z = (bounds.min.z - start.z) * inv_dir_z;
    float t1_z = (bounds.max.z - start.z) * inv_dir_z;
    if (inv_dir_z < 0.0f) {
        std::swap(t0_z, t1_z);
    }
    t_min = t0_z > t_min ? t0_z : t_min;
    t_max = t1_z < t_max ? t1_z : t_max;
    if (t_max < t_min) return false;

    t_out = t_min;

    return true;
}

}  // namespace vw::spatial

#endif  // VW_SPATIAL_RAY_INL_H
