#pragma once

#ifndef VW_GFX_SPATIAL_PLANE_H
#define VW_GFX_SPATIAL_PLANE_H

#include "vw/core.h"

namespace vw::gfx {

struct plane {
    vec3f normal;
    float distance;
    
    [[nodiscard]] float distance_to_point(const vec3f& point) const;
};

}  // namespace vw::gfx

#include "vw/gfx/spatial/plane.inl.h"

#endif  // VW_GFX_SPATIAL_PLANE_H
