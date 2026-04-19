#pragma once

#ifndef VW_SPATIAL_RAY_H
#define VW_SPATIAL_RAY_H

#include "vw/core.h"

namespace vw::spatial {

struct aabb;

struct ray {
    vec3f start;
    vec3f end;
    vec3f direction;

    ray(const vec3f& start, const vec3f& end);

    [[nodiscard]] float length() const;
    [[nodiscard]] vec3f point_at(float t) const;
    [[nodiscard]] bool intersects_at(const aabb& bounds, float& t_out) const;
};

}  // namespace vw::spatial

#include "vw/spatial/ray.inl.h"

#endif  // VW_SPATIAL_RAY_H
