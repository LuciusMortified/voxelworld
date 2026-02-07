#pragma once

#ifndef VW_GFX_ANIMATION_CLIP_H
#define VW_GFX_ANIMATION_CLIP_H

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "vw/core/transform.h"
#include "vw/core/types.h"
#include "vw/gfx/animation/animation_track.h"

namespace vw::gfx {

class animation_clip final {
public:
    explicit animation_clip(std::string name);

    void add_track(animation_track track);
    [[nodiscard]] auto get_track(std::string_view target_name) const -> const animation_track*;
    [[nodiscard]] auto get_track_mut(std::string_view target_name) -> animation_track*;
    [[nodiscard]] auto has_track(std::string_view target_name) const -> bool;
    [[nodiscard]] auto get_tracks() const -> const std::vector<animation_track>&;
    void remove_track(std::string_view target_name);
    void add_channel_to_track(std::string_view target_name, animation_channel_variant channel);
    [[nodiscard]] auto get_duration() const -> float32;
    [[nodiscard]] auto get_name() const -> const std::string&;

private:
    std::string name_;
    std::vector<animation_track> tracks_;
};

}  // namespace vw::gfx

#include "vw/gfx/animation/animation_clip.inl.h"

#endif  // VW_GFX_ANIMATION_CLIP_H
