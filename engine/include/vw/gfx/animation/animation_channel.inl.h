#pragma once

#include <algorithm>

namespace vw::gfx {

template <typename T>
void animation_channel<T>::add(const keyframe<T>& keyframe) {
    keyframes_.push_back(keyframe);
    std::sort(keyframes_.begin(), keyframes_.end());
}

template <typename T>
void animation_channel<T>::set_keyframes(std::vector<keyframe<T>> keyframes) {
    keyframes_ = std::move(keyframes);
    std::sort(keyframes_.begin(), keyframes_.end());
}

template <typename T>
auto animation_channel<T>::evaluate(float32 time) const -> std::expected<T, vw::animation_channel_error> {
    if (keyframes_.empty()) {
        return std::unexpected(vw::animation_channel_error::empty);
    }

    if (keyframes_.size() == 1) {
        return keyframes_[0].value;
    }

    if (time <= keyframes_.front().time) {
        return keyframes_.front().value;
    }

    if (time >= keyframes_.back().time) {
        return keyframes_.back().value;
    }

    auto it = std::lower_bound(
        keyframes_.begin(),
        keyframes_.end(),
        time,
        [](const keyframe<T>& kf, float32 t) { return kf.time < t; }
    );

    if (it == keyframes_.end()) {
        return keyframes_.back().value;
    }

    if (it == keyframes_.begin()) {
        return keyframes_.front().value;
    }

    auto prev_it = it - 1;

    const keyframe<T>& kf0 = *prev_it;
    const keyframe<T>& kf1 = *it;

    float32 duration = kf1.time - kf0.time;
    if (duration <= 0.0f) {
        return kf0.value;
    }

    float32 t = (time - kf0.time) / duration;

    return math::interpolate(kf0.value, kf1.value, t, kf0.interp, kf0.tangent_in, kf0.tangent_out);
}

template <typename T>
auto animation_channel<T>::get_duration() const -> std::expected<float32, vw::animation_channel_error> {
    if (keyframes_.empty()) {
        return std::unexpected(vw::animation_channel_error::empty);
    }
    return keyframes_.back().time;
}

}  // namespace vw::gfx
