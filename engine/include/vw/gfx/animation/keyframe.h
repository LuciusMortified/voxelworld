#pragma once

#ifndef VW_GFX_ANIMATION_KEYFRAME_H
#define VW_GFX_ANIMATION_KEYFRAME_H

#include "vw/core/math.h"
#include "vw/core/types.h"
#include "vw/gfx/animation/animation_types.h"

namespace vw::gfx {

template <typename T>
struct keyframe {
    float32 time;
    T value;
    math::interpolation_type interp = math::interpolation_type::linear;
    float32 tangent_in = 0.0f;
    float32 tangent_out = 1.0f;

    [[nodiscard]] auto operator<(const keyframe& other) const -> bool;
    [[nodiscard]] auto operator==(const keyframe& other) const -> bool;
};

using keyframe_vec3f = keyframe<vec3f>;
using keyframe_quat = keyframe<quat>;

}  // namespace vw::gfx

#include "vw/gfx/animation/keyframe.inl.h"

#endif  // VW_GFX_ANIMATION_KEYFRAME_H
