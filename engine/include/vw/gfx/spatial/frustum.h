#pragma once

#ifndef VW_GFX_SPATIAL_FRUSTUM_H
#define VW_GFX_SPATIAL_FRUSTUM_H

#include "vw/core.h"
#include "vw/gfx/spatial/plane.h"

namespace vw::gfx {

struct aabb;
struct ray;

struct frustum {
    // 6 плоскостей frustum (left, right, top, bottom, near, far)
    plane planes[6];

    // Создать frustum из view-projection матрицы
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

}  // namespace vw::gfx

#include "vw/gfx/spatial/frustum.inl.h"

#endif  // VW_GFX_SPATIAL_FRUSTUM_H
