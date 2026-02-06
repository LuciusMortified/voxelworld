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

    struct compile_settings {
        uint32 fps = 60;
        bool compile_matrices = true;
    };

    void compile(const compile_settings& settings);
    void compile();
    void clear_compiled();
    [[nodiscard]] auto is_compiled() const -> bool { return is_compiled_; }
    [[nodiscard]] auto get_compiled_fps() const -> uint32 { return compiled_fps_; }
    [[nodiscard]] auto evaluate(float32 time) const -> transform;
    [[nodiscard]] auto get_compiled_transform(float32 time) const -> const transform&;
    [[nodiscard]] auto get_compiled_matrix(float32 time) const -> const mat4f&;
    [[nodiscard]] auto get_duration() const -> float32;
    void add_channel(const animation_channel& channel);
    [[nodiscard]] auto get_channel(animation_property prop) const -> const animation_channel*;
    [[nodiscard]] auto has_channel(animation_property prop) const -> bool;

private:
    bool is_compiled_ = false;
    uint32 compiled_fps_ = 0;
    float32 frame_time_ = 0.0f;
    std::vector<transform> compiled_transforms_;
    std::vector<mat4f> compiled_local_matrices_;
};

}  // namespace vw::gfx

#include "vw/gfx/animation/animation_track.inl.h"

#endif  // VW_GFX_ANIMATION_TRACK_H
