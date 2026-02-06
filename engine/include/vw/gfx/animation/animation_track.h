#pragma once

#ifndef VW_GFX_ANIMATION_TRACK_H
#define VW_GFX_ANIMATION_TRACK_H

#include <string>
#include <vector>

#include "vw/core/math.h"
#include "vw/core/transform.h"
#include "vw/core/types.h"
#include "vw/gfx/animation/animation_channel.h"

namespace vw::gfx {

struct animation_track {
    std::string target_name;
    std::vector<animation_channel> channels;

    [[nodiscard]] auto evaluate(float32 time) const -> transform;
    [[nodiscard]] auto get_matrix(float32 time) const -> const mat4f&;
    [[nodiscard]] auto get_duration() const -> float32;
    void add_channel(const animation_channel& channel);
    [[nodiscard]] auto get_channel(animation_property prop) const -> const animation_channel*;
    [[nodiscard]] auto has_channel(animation_property prop) const -> bool;

private:
    void recompile_if_needed() const;

    mutable bool is_dirty_ = true;
    mutable uint32 compiled_fps_ = 60;
    mutable float32 frame_time_ = 0.0f;
    mutable std::vector<transform> compiled_transforms_;
    mutable std::vector<mat4f> compiled_matrices_;
};

}  // namespace vw::gfx

#include "vw/gfx/animation/animation_track.inl.h"

#endif  // VW_GFX_ANIMATION_TRACK_H
