#pragma once

#ifndef VW_SPATIAL_FRUSTUM_H
#define VW_SPATIAL_FRUSTUM_H

#include "vw/core.h"
#include "vw/spatial/plane.h"

namespace vw::spatial {

struct aabb;
struct ray;

struct frustum {
    plane planes[6];

    static frustum from_view_projection_matrix(const mat4f& view_proj);

    [[nodiscard]] bool intersects(const aabb& bounds) const;
    [[nodiscard]] bool intersects(const vec3f& point) const;
    [[nodiscard]] bool intersects(const ray& r) const;

    [[nodiscard]] bool operator==(const frustum& other) const;
    [[nodiscard]] bool operator!=(const frustum& other) const;

    [[nodiscard]] bool approximately_equal(
        const frustum& other, float angle_threshold, float distance_threshold
    ) const;
};

}  // namespace vw::spatial

#include "vw/spatial/frustum.inl.h"

#endif  // VW_SPATIAL_FRUSTUM_H
