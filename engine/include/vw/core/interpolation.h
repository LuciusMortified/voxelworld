#pragma once

#ifndef VW_CORE_INTERPOLATION_H
#define VW_CORE_INTERPOLATION_H

#include "vw/core/math.h"
#include "vw/core/types.h"
#include "vw/core/vec3.h"

namespace vw {

[[nodiscard]] auto ease_in(const vec3f& a, const vec3f& b, float32 t) -> vec3f;

[[nodiscard]] auto ease_out(const vec3f& a, const vec3f& b, float32 t) -> vec3f;

[[nodiscard]] auto ease_in_out(const vec3f& a, const vec3f& b, float32 t) -> vec3f;

[[nodiscard]] auto cubic_bezier(float32 t, float32 p0, float32 p1, float32 p2, float32 p3)
    -> float32;

[[nodiscard]] auto cubic_bezier(
    const vec3f& a,
    const vec3f& b,
    float32 t,
    float32 control1,
    float32 control2
) -> vec3f;

[[nodiscard]] auto apply_easing(float32 t, interpolation_type type) -> float32;

[[nodiscard]] auto apply_easing_bezier(
    float32 t,
    interpolation_type type,
    float32 control1,
    float32 control2
) -> float32;

[[nodiscard]] auto interpolate(
    const vec3f& a,
    const vec3f& b,
    float32 t,
    interpolation_type type,
    float32 control1 = 0.0f,
    float32 control2 = 1.0f
) -> vec3f;

}  // namespace vw

#include "vw/core/interpolation.inl.h"

#endif  // VW_CORE_INTERPOLATION_H
