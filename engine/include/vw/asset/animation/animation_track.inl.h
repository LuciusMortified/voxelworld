#pragma once

#include <algorithm>
#include <cmath>

namespace vw::asset {

inline animation_track::animation_track(
    std::string target_name, float32 fps
)
    : target_name_(std::move(target_name)), compiled_fps_(fps) {}

template <animation_property Prop>
void animation_track::add(
    animation_channel_for<Prop> channel
) {
    add_impl(animation_channel_variant(std::move(channel)));
}

inline void animation_track::recompile_if_needed() const {
    if (!is_dirty_) {
        return;
    }

    cached_duration_ = 0.0f;
    for (const auto& channel_var : channels_) {
        std::visit(
            [&](const auto& channel) {
                auto duration_result = channel.get_duration();
                if (duration_result) {
                    float32 channel_duration = *duration_result;
                    if (channel_duration > cached_duration_) {
                        cached_duration_ = channel_duration;
                    }
                }
            },
            channel_var
        );
    }

    if (cached_duration_ <= 0.0f && channels_.empty()) {
        is_dirty_ = false;
        return;
    }

    frame_time_ = 1.0f / compiled_fps_;

    uint32 frame_count = cached_duration_ > 0.0f
                             ? static_cast<uint32>(std::ceil(cached_duration_ / frame_time_)) + 1
                             : 1;

    compiled_transforms_.clear();
    compiled_transforms_.reserve(frame_count);

    for (uint32 i = 0; i < frame_count; ++i) {
        float32 time = static_cast<float32>(i) * frame_time_;

        transform t;

        for (const auto& channel_var : channels_) {
            std::visit(
                [&](const auto& channel) {
                    auto value_result = channel.evaluate(time);
                    if (!value_result) {
                        return;
                    }

                    auto property = channel.get_property();
                    auto value    = value_result.value();

                    if constexpr (std::is_same_v<decltype(value), vec3f>) {
                        switch (property) {
                            case animation_property::position:
                                t.set_position(value);
                                break;
                            case animation_property::scale:
                                t.set_scale(value);
                                break;
                            case animation_property::origin:
                                t.set_origin(value);
                                break;
                            default:
                                break;
                        }
                    } else if constexpr (std::is_same_v<decltype(value), quat>) {
                        t.set_rotation(value);
                    }
                },
                channel_var
            );
        }

        compiled_transforms_.push_back(t);
    }

    is_dirty_ = false;
}

inline auto animation_track::get_transform(
    float32 time
) const -> std::expected<transform, animation_track::error_type> {
    recompile_if_needed();

    if (compiled_transforms_.empty()) {
        return std::unexpected(error_type::empty);
    }

    if (compiled_transforms_.size() == 1) {
        return compiled_transforms_[0];
    }

    float32 frame_f  = time / frame_time_;
    auto frame_index = static_cast<uint32>(frame_f);

    if (frame_index >= compiled_transforms_.size() - 1) {
        return compiled_transforms_.back();
    }

    float32 frac = frame_f - static_cast<float32>(frame_index);

    return math::lerp(compiled_transforms_[frame_index], compiled_transforms_[frame_index + 1], frac);
}


inline auto animation_track::get_duration() const -> float32 {
    recompile_if_needed();
    return cached_duration_;
}

inline auto animation_track::get_frame_time() const -> float32 {
    recompile_if_needed();
    return frame_time_;
}

inline void animation_track::add_impl(
    animation_channel_variant channel
) {
    animation_property prop;
    std::visit([&](const auto& ch) { prop = ch.get_property(); }, channel);

    for (auto& existing_channel : channels_) {
        bool should_replace = false;
        std::visit(
            [&](const auto& ch) {
                if (ch.get_property() == prop) {
                    should_replace = true;
                }
            },
            existing_channel
        );

        if (should_replace) {
            existing_channel = std::move(channel);
            is_dirty_        = true;
            return;
        }
    }

    channels_.push_back(std::move(channel));
    is_dirty_ = true;
}

inline void animation_track::remove_channel(
    animation_property prop
) {
    auto it =
        std::ranges::remove_if(
            channels_,  //
            [prop](const auto& channel_var) {
                bool matches = false;
                std::visit(
                    [&](const auto& ch) {
                        if (ch.get_property() == prop) {
                            matches = true;
                        }
                    },
                    channel_var
                );
                return matches;
            }
        ).begin();

    if (it != channels_.end()) {
        channels_.erase(it, channels_.end());
        is_dirty_ = true;
    }
}

inline auto animation_track::get_channel(
    animation_property prop
) const -> const animation_channel_variant* {
    for (const auto& channel_var : channels_) {
        bool found = false;
        std::visit(
            [&](const auto& channel) {
                if (channel.get_property() == prop) {
                    found = true;
                }
            },
            channel_var
        );

        if (found) {
            return &channel_var;
        }
    }
    return nullptr;
}

inline auto animation_track::get_channel_mut(
    animation_property prop
) -> animation_channel_variant* {
    for (auto& channel_var : channels_) {
        bool found = false;
        std::visit(
            [&](const auto& channel) {
                if (channel.get_property() == prop) {
                    found = true;
                }
            },
            channel_var
        );

        if (found) {
            return &channel_var;
        }
    }
    return nullptr;
}

inline auto animation_track::has_channel(
    animation_property prop
) const -> bool {
    return get_channel(prop) != nullptr;
}

inline auto animation_track::get_channels() const
    -> const std::vector<animation_channel_variant>& {
    return channels_;
}

}  // namespace vw::asset
