#pragma once

#ifndef VW_GFX_ANIMATION_KEYFRAME_H
#define VW_GFX_ANIMATION_KEYFRAME_H

#include <limits>

#include "vw/core/math.h"
#include "vw/core/types.h"
#include "vw/gfx/animation/animation_types.h"

namespace vw::gfx {

template <typename T>
class animation_channel;

template <typename T>
struct keyframe {
    static constexpr uint32 invalid_id = std::numeric_limits<uint32>::max();

    keyframe() = default;
    keyframe(float32 time, T value)
        : time(time), value(std::move(value)) {}

    float32 time = 0.0f;
    T value{};
    math::interpolation_type interp = math::interpolation_type::linear;
    float32 tangent_in = 0.0f;
    float32 tangent_out = 1.0f;

    [[nodiscard]] auto id() const -> uint32 { return id_; }

    [[nodiscard]] auto operator<(const keyframe& other) const -> bool;
    [[nodiscard]] auto operator==(const keyframe& other) const -> bool;

private:
    friend class animation_channel<T>;
    uint32 id_ = invalid_id;
};

using keyframe_vec3f = keyframe<vec3f>;
using keyframe_quat = keyframe<quat>;

}  // namespace vw::gfx

#include "vw/gfx/animation/keyframe.inl.h"

#endif  // VW_GFX_ANIMATION_KEYFRAME_H
