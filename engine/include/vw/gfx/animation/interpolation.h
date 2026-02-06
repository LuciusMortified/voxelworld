#pragma once

#ifndef VW_GFX_ANIMATION_INTERPOLATION_H
#define VW_GFX_ANIMATION_INTERPOLATION_H

#include "vw/core/math.h"
#include "vw/core/types.h"
#include "vw/core/vec2.h"
#include "vw/gfx/animation/animation_types.h"

namespace vw::gfx {

[[nodiscard]] auto lerp(const vec3f& a, const vec3f& b, float32 t) -> vec3f;

[[nodiscard]] auto ease_in(const vec3f& a, const vec3f& b, float32 t) -> vec3f;

[[nodiscard]] auto ease_out(const vec3f& a, const vec3f& b, float32 t) -> vec3f;

[[nodiscard]] auto ease_in_out(const vec3f& a, const vec3f& b, float32 t) -> vec3f;

[[nodiscard]] auto cubic_bezier(
    const vec3f& a,
    const vec3f& b,
    float32 t,
    const vec2f& control1,
    const vec2f& control2
) -> vec3f;

[[nodiscard]] auto apply_easing(float32 t, interpolation_type type) -> float32;

[[nodiscard]] auto apply_easing_bezier(
    float32 t,
    interpolation_type type,
    const vec2f& control1,
    const vec2f& control2
) -> float32;

[[nodiscard]] auto interpolate(
    const vec3f& a,
    const vec3f& b,
    float32 t,
    interpolation_type type,
    const vec2f& control1 = vec2f{0.0f, 0.0f},
    const vec2f& control2 = vec2f{1.0f, 1.0f}
) -> vec3f;

}  // namespace vw::gfx

#include "vw/gfx/animation/interpolation.inl.h"

#endif  // VW_GFX_ANIMATION_INTERPOLATION_H
