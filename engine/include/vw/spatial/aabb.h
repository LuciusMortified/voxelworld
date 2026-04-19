#pragma once

#ifndef VW_SPATIAL_AABB_H
#define VW_SPATIAL_AABB_H

#include "vw/core.h"

namespace vw::spatial {

struct ray;

struct aabb {
    vec3f min;
    vec3f max;

    [[nodiscard]] vec3f center() const;
    [[nodiscard]] vec3f size() const;
    [[nodiscard]] float area() const;
    [[nodiscard]] bool intersects(const aabb& other) const;
    [[nodiscard]] bool intersects(const ray& r) const;
    [[nodiscard]] bool intersects(const vec3f& point) const;

    [[nodiscard]] static aabb merge(const aabb& a, const aabb& b);

    [[nodiscard]] bool operator==(const aabb& other) const;
    [[nodiscard]] bool operator!=(const aabb& other) const;
};

}  // namespace vw::spatial

#include "vw/spatial/aabb.inl.h"

#endif  // VW_SPATIAL_AABB_H
