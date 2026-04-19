#pragma once

#include <algorithm>

namespace vw::asset {

template <typename T>
void animation_channel<T>::add(const keyframe<T>& kf) {
    keyframes_.push_back(kf);
    if (keyframes_.back().id_ == invalid_keyframe_id) {
        keyframes_.back().id_ = next_id_++;
    }
    std::sort(keyframes_.begin(), keyframes_.end());
}

template <typename T>
void animation_channel<T>::replace(uint32 id, const keyframe<T>& new_kf) {
    auto it = std::find_if(keyframes_.begin(), keyframes_.end(), [id](const auto& kf) {
        return kf.id_ == id;
    });
    if (it != keyframes_.end()) {
        uint32 preserved_id = it->id_;
        *it = new_kf;
        it->id_ = preserved_id;
        std::sort(keyframes_.begin(), keyframes_.end());
    }
}

template <typename T>
void animation_channel<T>::remove(uint32 id) {
    std::erase_if(keyframes_, [id](const auto& kf) {
        return kf.id_ == id;
    });
}

template <typename T>
void animation_channel<T>::set_keyframes(std::vector<keyframe<T>> keyframes) {
    keyframes_ = std::move(keyframes);
    next_id_ = 0;
    for (auto& kf : keyframes_) {
        kf.id_ = next_id_++;
    }
    std::sort(keyframes_.begin(), keyframes_.end());
}

template <typename T>
void animation_channel<T>::clear() {
    keyframes_.clear();
}

template <typename T>
auto animation_channel<T>::evaluate(float32 time) const -> std::expected<T, error_type> {
    if (keyframes_.empty()) {
        return std::unexpected(error_type::empty);
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
auto animation_channel<T>::get_duration() const -> std::expected<float32, animation_channel<T>::error_type> {
    if (keyframes_.empty()) {
        return std::unexpected(error_type::empty);
    }
    return keyframes_.back().time;
}

template <animation_property Prop>
auto make_animation_channel() -> animation_channel_for<Prop> {
    return animation_channel_for<Prop>(Prop);
}

}  // namespace vw::asset
