#pragma once

#include <algorithm>

namespace vw::gfx {

inline animation_clip::animation_clip(std::string name) : name_(std::move(name)) {}

inline void animation_clip::add_track(animation_track track) {
    tracks_.push_back(std::move(track));
}

inline auto animation_clip::get_track(const std::string& target_name) const
    -> const animation_track* {
    for (const auto& track : tracks_) {
        if (track.target_name == target_name) {
            return &track;
        }
    }
    return nullptr;
}

inline auto animation_clip::get_track_mut(const std::string& target_name) -> animation_track* {
    for (auto& track : tracks_) {
        if (track.target_name == target_name) {
            return &track;
        }
    }
    return nullptr;
}

inline auto animation_clip::has_track(const std::string& target_name) const -> bool {
    return get_track(target_name) != nullptr;
}

inline auto animation_clip::get_tracks() const -> const std::vector<animation_track>& {
    return tracks_;
}

inline void animation_clip::remove_track(const std::string& target_name) {
    tracks_.erase(
        std::remove_if(
            tracks_.begin(),
            tracks_.end(),
            [&target_name](const animation_track& track) {
                return track.target_name == target_name;
            }
        ),
        tracks_.end()
    );
}

inline void animation_clip::add_channel_to_track(
    const std::string& target_name,
    const animation_channel& channel
) {
    animation_track* track = get_track_mut(target_name);

    if (track == nullptr) {
        animation_track new_track;
        new_track.target_name = target_name;
        new_track.add_channel(channel);
        add_track(std::move(new_track));
    } else {
        track->add_channel(channel);
    }
}

inline auto animation_clip::get_duration() const -> float32 {
    float32 max_duration = 0.0f;

    for (const auto& track : tracks_) {
        float32 track_duration = track.get_duration();
        if (track_duration > max_duration) {
            max_duration = track_duration;
        }
    }

    return max_duration;
}

inline auto animation_clip::get_name() const -> const std::string& {
    return name_;
}

inline void animation_clip::set_name(std::string name) {
    name_ = std::move(name);
}

inline void animation_clip::add_channel(const animation_channel& channel) {
    add_channel_to_track("", channel);
}

inline auto animation_clip::evaluate_single(float32 time) const -> transform {
    const animation_track* root_track = get_track("");

    if (root_track == nullptr) {
        return transform{};
    }

    return root_track->evaluate(time);
}

}  // namespace vw::gfx
