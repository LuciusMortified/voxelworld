#pragma once

#ifndef VW_GFX_WORLD_COMPONENTS_SPATIAL_COMPONENT_H
#define VW_GFX_WORLD_COMPONENTS_SPATIAL_COMPONENT_H

#include "vw/core.h"
#include "vw/gfx/world/spatial_layer.h"
#include "vw/spatial/aabb.h"

namespace vw::gfx {

using vw::spatial::aabb;

template <typename>
class spatial_system;

struct spatial_component final {
private:
    aabb bounds_;
    aabb fat_bounds_;
    spatial_layer_mask layer_ = spatial_layer::prop;
    bool dirty_   = true;

public:
    [[nodiscard]] auto get_bounds() const -> const aabb&;
    [[nodiscard]] auto get_layer() const -> spatial_layer_mask;
    [[nodiscard]] auto is_dirty() const -> bool;

    template <typename>
    friend class spatial_system;
};

}  // namespace vw::gfx

#include "vw/gfx/world/components/spatial_component.inl.h"

#endif  // VW_GFX_WORLD_COMPONENTS_SPATIAL_COMPONENT_H
