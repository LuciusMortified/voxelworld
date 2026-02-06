#pragma once

#include <algorithm>

#include "vw/core/interpolation.h"

namespace vw::gfx {

inline auto animation_channel::evaluate(float32 time) const -> vec3f {
    if (keyframes.empty()) {
        return vec3f{0.0f, 0.0f, 0.0f};
    }

    if (keyframes.size() == 1) {
        return keyframes[0].value;
    }

    if (time <= keyframes.front().time) {
        return keyframes.front().value;
    }

    if (time >= keyframes.back().time) {
        return keyframes.back().value;
    }

    auto it = std::lower_bound(
        keyframes.begin(),
        keyframes.end(),
        time,
        [](const keyframe_vec3& kf, float32 t) { return kf.time < t; }
    );

    if (it == keyframes.end()) {
        return keyframes.back().value;
    }

    if (it == keyframes.begin()) {
        return keyframes.front().value;
    }

    auto prev_it = it - 1;

    const keyframe_vec3& kf0 = *prev_it;
    const keyframe_vec3& kf1 = *it;

    float32 duration = kf1.time - kf0.time;
    if (duration <= 0.0f) {
        return kf0.value;
    }

    float32 t = (time - kf0.time) / duration;

    return vw::interpolate(kf0.value, kf1.value, t, kf0.interp, kf0.tangent_in, kf0.tangent_out);
}

inline auto animation_channel::get_duration() const -> float32 {
    if (keyframes.empty()) {
        return 0.0f;
    }
    return keyframes.back().time;
}

inline void animation_channel::add_keyframe(const keyframe_vec3& keyframe) {
    keyframes.push_back(keyframe);
    std::sort(keyframes.begin(), keyframes.end());
}

}  // namespace vw::gfx
