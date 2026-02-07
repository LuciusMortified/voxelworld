#pragma once

#ifndef VW_GFX_ANIMATION_TARGET_COMPONENT_H
#define VW_GFX_ANIMATION_TARGET_COMPONENT_H

#include <string>

namespace vw::gfx {

template <typename... Cs>
class animation_system;

struct animation_target_component final {
public:
    explicit animation_target_component(std::string name) : target_name_(std::move(name)) {}

    [[nodiscard]] auto get_name() const -> const std::string&;

private:
    std::string target_name_;

    template <typename... Cs>
    friend class animation_system;
};

}  // namespace vw::gfx

#include "vw/gfx/world/components/animation_target_component.inl.h"

#endif  // VW_GFX_ANIMATION_TARGET_COMPONENT_H
