#pragma once

#ifndef VW_GFX_ANIMATION_TRACK_H
#define VW_GFX_ANIMATION_TRACK_H

#include <expected>
#include <string>
#include <vector>

#include "vw/core/math.h"
#include "vw/core/transform.h"
#include "vw/core/types.h"
#include "vw/gfx/animation/animation_channel.h"

namespace vw::gfx {

class animation_track final {
public:
    enum class error {
        empty
    };

    explicit animation_track(std::string target_name, uint32 fps = 60);

    void add(const animation_channel3f& channel);
    [[nodiscard]] auto get_transform(float32 time) const -> std::expected<transform, error>;
    [[nodiscard]] auto get_matrix(float32 time) const -> std::expected<mat4f, error>;
    [[nodiscard]] auto get_duration() const -> float32;
    [[nodiscard]] auto get_target_name() const -> const std::string& { return target_name_; }
    [[nodiscard]] auto get_channel(animation_property prop) const -> const animation_channel3f*;
    [[nodiscard]] auto has_channel(animation_property prop) const -> bool;

private:
    void recompile_if_needed() const;

    std::string target_name_;
    std::vector<animation_channel3f> channels_;

    mutable bool is_dirty_ = true;
    mutable uint32 compiled_fps_;
    mutable float32 frame_time_ = 0.0f;
    mutable std::vector<transform> compiled_transforms_;
    mutable std::vector<mat4f> compiled_matrices_;
};

}  // namespace vw::gfx

#include "vw/gfx/animation/animation_track.inl.h"

#endif  // VW_GFX_ANIMATION_TRACK_H
