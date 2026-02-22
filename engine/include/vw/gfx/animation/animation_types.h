#pragma once

#ifndef VW_GFX_ANIMATION_TYPES_H
#define VW_GFX_ANIMATION_TYPES_H

#include "vw/core/math.h"
#include "vw/core/quat.h"
#include "vw/core/types.h"
#include "vw/core/vec3.h"

namespace vw::gfx {

enum class animation_state : uint8 {
    stopped,
    playing,
    paused
};

enum class animation_loop_mode : uint8 {
    once,
    loop,
    ping_pong
};

enum class animation_property : uint8 {
    position,
    rotation,
    scale,
    origin
};

struct transition {
    float32 duration = 0.0f;
    math::interpolation_type interp = math::interpolation_type::linear;
    float32 tangent_in = 0.0f;
    float32 tangent_out = 1.0f;
};

template <animation_property Prop>
struct animation_property_traits;

template <>
struct animation_property_traits<animation_property::position> {
    using type = vec3f;
};

template <>
struct animation_property_traits<animation_property::rotation> {
    using type = quat;
};

template <>
struct animation_property_traits<animation_property::scale> {
    using type = vec3f;
};

template <>
struct animation_property_traits<animation_property::origin> {
    using type = vec3f;
};

}  // namespace vw::gfx

#endif  // VW_GFX_ANIMATION_TYPES_H
