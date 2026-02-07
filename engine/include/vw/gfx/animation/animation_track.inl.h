#pragma once

#include <algorithm>
#include <cmath>

namespace vw::gfx {

inline animation_track::animation_track(std::string target_name, uint32 fps)
    : target_name_(std::move(target_name)), compiled_fps_(fps) {}

inline void animation_track::recompile_if_needed() const {
    if (!is_dirty_) {
        return;
    }

    float32 duration = get_duration();
    if (duration <= 0.0f) {
        is_dirty_ = false;
        return;
    }

    frame_time_ = 1.0f / static_cast<float32>(compiled_fps_);
    uint32 frame_count = static_cast<uint32>(std::ceil(duration / frame_time_)) + 1;

    compiled_transforms_.clear();
    compiled_transforms_.reserve(frame_count);
    compiled_matrices_.clear();
    compiled_matrices_.reserve(frame_count);

    for (uint32 i = 0; i < frame_count; ++i) {
        float32 time = static_cast<float32>(i) * frame_time_;

        transform t;

        for (const auto& channel : channels_) {
            auto value_result = channel.evaluate(time);
            if (!value_result) {
                continue;
            }
            vec3f value = *value_result;

            switch (channel.get_property()) {
                case animation_property::position:
                    t.set_position(value);
                    break;
                case animation_property::rotation:
                    t.set_rotation(value);
                    break;
                case animation_property::scale:
                    t.set_scale(value);
                    break;
                case animation_property::origin:
                    t.set_origin(value);
                    break;
            }
        }

        compiled_transforms_.push_back(t);
        compiled_matrices_.push_back(t.calc_matrix());
    }

    is_dirty_ = false;
}

inline auto animation_track::get_transform(float32 time) const -> std::expected<transform, animation_track::error_type> {
    recompile_if_needed();

    if (compiled_transforms_.empty()) {
        return std::unexpected(animation_track::error_type::empty);
    }

    uint32 frame_index = static_cast<uint32>(time / frame_time_);

    if (frame_index >= compiled_transforms_.size()) {
        frame_index = static_cast<uint32>(compiled_transforms_.size()) - 1;
    }

    return compiled_transforms_[frame_index];
}

inline auto animation_track::get_matrix(float32 time) const -> std::expected<mat4f, animation_track::error_type> {
    recompile_if_needed();

    if (compiled_matrices_.empty()) {
        return std::unexpected(animation_track::error_type::empty);
    }

    uint32 frame_index = static_cast<uint32>(time / frame_time_);

    if (frame_index >= compiled_matrices_.size()) {
        frame_index = static_cast<uint32>(compiled_matrices_.size()) - 1;
    }

    return compiled_matrices_[frame_index];
}

inline auto animation_track::get_duration() const -> float32 {
    float32 max_duration = 0.0f;

    for (const auto& channel : channels_) {
        auto duration_result = channel.get_duration();
        if (!duration_result) {
            continue;
        }
        float32 channel_duration = *duration_result;
        if (channel_duration > max_duration) {
            max_duration = channel_duration;
        }
    }

    return max_duration;
}

inline void animation_track::add(const animation_channel_vec3f& channel) {
    channels_.push_back(channel);
    is_dirty_ = true;
}

inline auto animation_track::get_channel(animation_property prop) const
    -> const animation_channel_vec3f* {
    for (const auto& channel : channels_) {
        if (channel.get_property() == prop) {
            return &channel;
        }
    }
    return nullptr;
}

inline auto animation_track::has_channel(animation_property prop) const -> bool {
    return get_channel(prop) != nullptr;
}

}  // namespace vw::gfx
