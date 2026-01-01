#pragma once

#ifndef VW_GFX_WORLD_COMPONENTS_SPATIAL_COMPONENT_H
#define VW_GFX_WORLD_COMPONENTS_SPATIAL_COMPONENT_H

#include "vw/core.h"
#include "vw/gfx/spatial/aabb.h"

namespace vw::gfx {

template <typename... Cs>
class spatial_system;

struct spatial_component final {
private:
    aabb bounds_;        // Реальный AABB
    aabb fat_bounds_;    // Расширенный AABB для оптимизации обновлений дерева
    bool dirty_ = true;  // Флаг для первоначальной вставки

public:
    [[nodiscard]] auto get_bounds() const -> const aabb&;
    [[nodiscard]] auto is_dirty() const -> bool;

    template <typename... Cs>
    friend class spatial_system;
};

}  // namespace vw::gfx

#include "vw/gfx/world/components/spatial_component.inl.h"

#endif  // VW_GFX_WORLD_COMPONENTS_SPATIAL_COMPONENT_H
