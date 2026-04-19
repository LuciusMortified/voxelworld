#pragma once

#ifndef VW_SPATIAL_PLANE_INL_H
#define VW_SPATIAL_PLANE_INL_H

#include "vw/core/math.h"
#include "vw/spatial/plane.h"

namespace vw::spatial {

inline float plane::distance_to_point(const vec3f& point) const {
    return vw::math::dot(normal, point) + distance;
}

}  // namespace vw::spatial

#endif  // VW_SPATIAL_PLANE_INL_H
