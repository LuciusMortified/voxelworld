#pragma once

#ifndef VW_GFX_ANIMATION_TARGET_COMPONENT_H
#define VW_GFX_ANIMATION_TARGET_COMPONENT_H

#include <string>

#include "vw/core.h"

namespace vw::gfx {

template <typename>
class animation_system;

struct animation_target_component final {
public:
    animation_target_component() = default;

    [[nodiscard]] auto get_name() const -> const std::string&;
    [[nodiscard]] auto get_rest_transform() const -> const transform&;

private:
    std::string target_name_;
    transform rest_transform_;

    template <typename>
    friend class animation_system;
};

}  // namespace vw::gfx

#include "vw/gfx/world/components/animation_target_component.inl.h"

#endif  // VW_GFX_ANIMATION_TARGET_COMPONENT_H
