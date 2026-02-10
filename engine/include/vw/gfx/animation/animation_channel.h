#pragma once

#ifndef VW_GFX_ANIMATION_CHANNEL_H
#define VW_GFX_ANIMATION_CHANNEL_H

#include <algorithm>
#include <expected>
#include <variant>
#include <vector>

#include "vw/core/math.h"
#include "vw/core/types.h"
#include "vw/gfx/animation/animation_types.h"
#include "vw/gfx/animation/keyframe.h"

namespace vw::gfx {

template <typename T>
class animation_channel final {
public:
    enum class error_type {
        empty
    };

    explicit animation_channel(animation_property property) : property_(property) {}

    void add(const keyframe<T>& keyframe);
    void set_keyframes(std::vector<keyframe<T>> keyframes);
    void clear();
    [[nodiscard]] auto evaluate(float32 time) const -> std::expected<T, error_type>;
    [[nodiscard]] auto get_duration() const -> std::expected<float32, error_type>;
    [[nodiscard]] auto get_property() const -> animation_property { return property_; }
    [[nodiscard]] auto is_empty() const -> bool { return keyframes_.empty(); }
    [[nodiscard]] auto keyframe_count() const -> size_t { return keyframes_.size(); }

private:
    animation_property property_;
    std::vector<keyframe<T>> keyframes_;
};

using animation_channel_variant = std::variant<
    animation_channel<vec3f>
>;

template <animation_property Prop>
using channel_for = animation_channel<typename animation_property_traits<Prop>::type>;

template <animation_property Prop>
auto make_channel() -> channel_for<Prop> {
    return channel_for<Prop>(Prop);
}

}  // namespace vw::gfx

#include "vw/gfx/animation/animation_channel.inl.h"

#endif  // VW_GFX_ANIMATION_CHANNEL_H
