#pragma once

#ifndef VW_ASSET_ANIMATION_CHANNEL_H
#define VW_ASSET_ANIMATION_CHANNEL_H

#include <algorithm>
#include <expected>
#include <variant>
#include <vector>

#include "vw/core/math.h"
#include "vw/core/types.h"
#include "vw/asset/animation/animation_types.h"
#include "vw/asset/animation/keyframe.h"

namespace vw::asset {

template <typename T>
class animation_channel final {
public:
    enum class error_type : uint8 { empty };

    explicit animation_channel(
        animation_property property
    )
        : property_(property) {}

    void add(const keyframe<T>& keyframe);
    void replace(uint32 id, const keyframe<T>& new_keyframe);
    void remove(uint32 id);
    void set_keyframes(std::vector<keyframe<T>> keyframes);
    void clear();
    [[nodiscard]] auto evaluate(float32 time) const -> std::expected<T, error_type>;
    [[nodiscard]] auto get_duration() const -> std::expected<float32, error_type>;
    [[nodiscard]] auto get_property() const -> animation_property {
        return property_;
    }
    [[nodiscard]] auto is_empty() const -> bool {
        return keyframes_.empty();
    }
    [[nodiscard]] auto keyframe_count() const -> size_t {
        return keyframes_.size();
    }
    [[nodiscard]] auto get_keyframes() const -> const std::vector<keyframe<T>>& {
        return keyframes_;
    }
    [[nodiscard]] auto get_keyframes_mut() -> std::vector<keyframe<T>>& {
        return keyframes_;
    }

private:
    animation_property property_;
    std::vector<keyframe<T>> keyframes_;
    uint32 next_id_ = 0;
};

using animation_channel_variant = std::variant<animation_channel<vec3f>, animation_channel<quat>>;

template <animation_property Prop>
using animation_channel_for = animation_channel<typename animation_property_traits<Prop>::type>;

template <animation_property Prop>
auto make_animation_channel() -> animation_channel_for<Prop>;

}  // namespace vw::asset

#include "vw/asset/animation/animation_channel.inl.h"

#endif  // VW_ASSET_ANIMATION_CHANNEL_H
