#pragma once

#ifndef VW_GFX_ANIMATION_CHANNEL_H
#define VW_GFX_ANIMATION_CHANNEL_H

#include <algorithm>
#include <vector>

#include "vw/core/math.h"
#include "vw/core/types.h"
#include "vw/gfx/animation/animation_types.h"
#include "vw/gfx/animation/keyframe.h"

namespace vw::gfx {

struct animation_channel {
    animation_property property;
    std::vector<keyframe_vec3> keyframes;

    [[nodiscard]] auto evaluate(float32 time) const -> vec3f;
    [[nodiscard]] auto get_duration() const -> float32;
    void add_keyframe(const keyframe_vec3& keyframe);
    [[nodiscard]] auto is_empty() const -> bool { return keyframes.empty(); }
    [[nodiscard]] auto keyframe_count() const -> size_t { return keyframes.size(); }
};

}  // namespace vw::gfx

#include "vw/gfx/animation/animation_channel.inl.h"

#endif  // VW_GFX_ANIMATION_CHANNEL_H
