#pragma once

#ifndef VW_ASSET_ANIMATION_KEYFRAME_H
#define VW_ASSET_ANIMATION_KEYFRAME_H

#include <limits>

#include "vw/core/math.h"
#include "vw/core/types.h"
#include "vw/asset/animation/animation_types.h"

namespace vw::asset {

inline constexpr uint32 invalid_keyframe_id = std::numeric_limits<uint32>::max();

template <typename T>
class animation_channel;

template <typename T>
struct keyframe {

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
    uint32 id_ = invalid_keyframe_id;
};

using keyframe_vec3f = keyframe<vec3f>;
using keyframe_quat = keyframe<quat>;

}  // namespace vw::asset

#include "vw/asset/animation/keyframe.inl.h"

#endif  // VW_ASSET_ANIMATION_KEYFRAME_H
