#pragma once

#ifndef VW_GFX_ANIMATION_LAYER_H
#define VW_GFX_ANIMATION_LAYER_H

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "vw/core/math.h"
#include "vw/core/transform.h"
#include "vw/core/types.h"
#include "vw/gfx/animation/animation_clip.h"
#include "vw/gfx/animation/animation_types.h"

namespace vw::gfx {

// Слой анимации — независимый плеер с маской targets
struct animation_layer {
    std::shared_ptr<animation_clip> clip;
    animation_state state         = animation_state::stopped;
    float32 time                  = 0.0f;
    float32 playback_speed        = 1.0f;
    animation_loop_mode loop_mode = animation_loop_mode::once;
    float32 direction             = 1.0f;

    std::shared_ptr<animation_clip> blend_prev_clip;
    float32 blend_prev_time           = 0.0f;
    float32 blend_prev_playback_speed = 1.0f;
    float32 blend_prev_direction      = 1.0f;
    float32 blend_elapsed             = 0.0f;
    transition blend_transition;
    std::unordered_map<std::string, transform> blend_snapshot;

    float32 fade_influence = 0.0f;
    float32 fade_elapsed   = 0.0f;
    transition fade_in;
    transition fade_out;
    bool fade_is_out = false;

    std::unordered_set<std::string> mask;

    [[nodiscard]] auto is_active() const -> bool;
    [[nodiscard]] auto affects_target(const std::string& name) const -> bool;
};

}  // namespace vw::gfx

#include "vw/gfx/animation/animation_layer.inl.h"

#endif  // VW_GFX_ANIMATION_LAYER_H
