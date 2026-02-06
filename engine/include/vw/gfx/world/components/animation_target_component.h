#pragma once

#ifndef VW_GFX_ANIMATION_TARGET_COMPONENT_H
#define VW_GFX_ANIMATION_TARGET_COMPONENT_H

#include <string>

namespace vw::gfx {

template <typename... Cs>
class animation_system;

// Компонент цели анимации - помечает entity как цель для анимационного трека
// Прикрепляется к каждой анимируемой части в иерархии (head, arms, legs, etc.)
struct animation_target_component final {
private:
    std::string target_name_;

public:
    [[nodiscard]] auto get_name() const -> const std::string&;

    template <typename... Cs>
    friend class animation_system;
};

}  // namespace vw::gfx

#include "vw/gfx/world/components/animation_target_component.inl.h"

#endif  // VW_GFX_ANIMATION_TARGET_COMPONENT_H
