#pragma once

#ifndef VW_GFX_SPATIAL_RAY_H
#define VW_GFX_SPATIAL_RAY_H

#include "vw/core.h"
#include "vw/gfx/world/entity.h"

namespace vw::gfx {

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

struct voxel_ray_hit {
    entity ent;
    vec3i voxel_pos;
    vec3i empty_pos;
};

}  // namespace vw::gfx

#include "vw/gfx/spatial/ray.inl.h"

#endif  // VW_GFX_SPATIAL_RAY_H
