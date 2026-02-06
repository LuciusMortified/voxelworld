#pragma once

#ifndef VW_GFX_ANIMATION_KEYFRAME_H
#define VW_GFX_ANIMATION_KEYFRAME_H

#include "vw/core/math.h"
#include "vw/core/types.h"
#include "vw/core/vec2.h"
#include "vw/gfx/animation/animation_types.h"

namespace vw::gfx {

struct keyframe_vec3 {
    float32 time;
    vec3f value;
    interpolation_type interp = interpolation_type::linear;
    vec2f tangent_in{0.0f, 0.0f};
    vec2f tangent_out{1.0f, 1.0f};

    [[nodiscard]] auto operator<(const keyframe_vec3& other) const -> bool {
        return time < other.time;
    }

    [[nodiscard]] auto operator==(const keyframe_vec3& other) const -> bool {
        return time == other.time;
    }
};

}  // namespace vw::gfx

#endif  // VW_GFX_ANIMATION_KEYFRAME_H
