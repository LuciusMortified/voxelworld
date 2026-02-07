#pragma once

#ifndef VW_GFX_ANIMATION_CHANNEL_H
#define VW_GFX_ANIMATION_CHANNEL_H

#include <algorithm>
#include <expected>
#include <vector>

#include "vw/core/math.h"
#include "vw/core/types.h"
#include "vw/gfx/animation/animation_types.h"
#include "vw/gfx/animation/keyframe.h"

namespace vw::gfx {

template <typename T>
class animation_channel final {
public:
    enum class error {
        empty
    };

    explicit animation_channel(animation_property property) : property_(property) {}

    void add(const keyframe<T>& keyframe);
    void set_keyframes(std::vector<keyframe<T>> keyframes);
    [[nodiscard]] auto evaluate(float32 time) const -> std::expected<T, error>;
    [[nodiscard]] auto get_duration() const -> std::expected<float32, error>;
    [[nodiscard]] auto get_property() const -> animation_property { return property_; }
    [[nodiscard]] auto is_empty() const -> bool { return keyframes_.empty(); }
    [[nodiscard]] auto keyframe_count() const -> size_t { return keyframes_.size(); }

private:
    animation_property property_;
    std::vector<keyframe<T>> keyframes_;
};

using animation_channel3f = animation_channel<vec3f>;

}  // namespace vw::gfx

#include "vw/gfx/animation/animation_channel.inl.h"

#endif  // VW_GFX_ANIMATION_CHANNEL_H
