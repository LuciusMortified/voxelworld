#pragma once

#ifndef VW_ASSET_ANIMATION_TRACK_H
#define VW_ASSET_ANIMATION_TRACK_H

#include <expected>
#include <string>
#include <vector>

#include "vw/core/math.h"
#include "vw/core/transform.h"
#include "vw/core/types.h"
#include "vw/asset/animation/animation_channel.h"

namespace vw::asset {

class animation_track final {
public:
    enum class error_type {
        empty
    };

    explicit animation_track(std::string target_name, float32 fps = 60.0f);

    template <animation_property Prop>
    void add(animation_channel_for<Prop> channel);

    void remove_channel(animation_property prop);

    [[nodiscard]] auto get_transform(float32 time) const -> std::expected<transform, error_type>;
    [[nodiscard]] auto get_duration() const -> float32;
    [[nodiscard]] auto get_frame_time() const -> float32;
    [[nodiscard]] auto get_target_name() const -> const std::string& { return target_name_; }
    [[nodiscard]] auto get_channel(animation_property prop) const -> const animation_channel_variant*;
    [[nodiscard]] auto get_channel_mut(animation_property prop) -> animation_channel_variant*;
    [[nodiscard]] auto has_channel(animation_property prop) const -> bool;
    [[nodiscard]] auto get_channels() const -> const std::vector<animation_channel_variant>&;
    [[nodiscard]] auto get_fps() const -> float32 { return compiled_fps_; }
    void mark_dirty() { is_dirty_ = true; }

private:
    void add_impl(animation_channel_variant channel);
    void recompile_if_needed() const;

    std::string target_name_;
    std::vector<animation_channel_variant> channels_;

    mutable bool is_dirty_ = true;
    mutable float32 compiled_fps_;
    mutable float32 frame_time_ = 0.0f;
    mutable float32 cached_duration_ = 0.0f;
    mutable std::vector<transform> compiled_transforms_;
};

}  // namespace vw::asset

#include "vw/asset/animation/animation_track.inl.h"

#endif  // VW_ASSET_ANIMATION_TRACK_H
