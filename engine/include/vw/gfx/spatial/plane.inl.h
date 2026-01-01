#pragma once

#ifndef VW_GFX_SPATIAL_PLANE_INL_H
#define VW_GFX_SPATIAL_PLANE_INL_H

#include "vw/gfx/spatial/plane.h"
#include "vw/core/math.h"

namespace vw::gfx {

inline float plane::distance_to_point(const vec3f& point) const {
    return vw::math::dot(normal, point) + distance;
}

}  // namespace vw::gfx

#endif  // VW_GFX_SPATIAL_PLANE_INL_H
