#pragma once

#ifndef VW_SPATIAL_PLANE_H
#define VW_SPATIAL_PLANE_H

#include "vw/core.h"

namespace vw::spatial {

struct plane {
    vec3f normal;
    float distance;

    [[nodiscard]] float distance_to_point(const vec3f& point) const;
};

}  // namespace vw::spatial

#include "vw/spatial/plane.inl.h"

#endif  // VW_SPATIAL_PLANE_H
