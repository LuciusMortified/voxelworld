#pragma once

#ifndef VW_GFX_ANIMATION_PLAYER_COMPONENT_H
#define VW_GFX_ANIMATION_PLAYER_COMPONENT_H

#include <vector>

#include "vw/core/types.h"
#include "vw/gfx/animation/animation_layer.h"

namespace vw::gfx {

template <typename>
class animation_system;

struct animation_player_component final {
private:
    std::vector<animation_layer> layers_;

public:
    [[nodiscard]] auto layer_count() const -> size_t;
    [[nodiscard]] auto has_layer(size_t index) const -> bool;
    [[nodiscard]] auto get_layer(size_t index) const -> const animation_layer&;
    [[nodiscard]] auto is_any_playing() const -> bool;

    template <typename>
    friend class animation_system;
};

}  // namespace vw::gfx

#include "vw/gfx/world/components/animation_player_component.inl.h"

#endif  // VW_GFX_ANIMATION_PLAYER_COMPONENT_H
