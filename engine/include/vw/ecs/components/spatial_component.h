#pragma once

#ifndef VW_ECS_COMPONENTS_SPATIAL_COMPONENT_H
#define VW_ECS_COMPONENTS_SPATIAL_COMPONENT_H

#include "vw/core.h"
#include "vw/ecs/spatial/spatial_layer.h"
#include "vw/spatial/aabb.h"

namespace vw::ecs {


class spatial_system;

struct spatial_component final {
private:
    vw::spatial::aabb bounds_;
    vw::spatial::aabb fat_bounds_;
    spatial_layer_mask layer_ = spatial_layer::prop;
    bool dirty_   = true;

public:
    [[nodiscard]] auto get_bounds() const -> const vw::spatial::aabb&;
    [[nodiscard]] auto get_layer() const -> spatial_layer_mask;
    [[nodiscard]] auto is_dirty() const -> bool;

        friend class spatial_system;
};

}  // namespace vw::ecs

#include "vw/ecs/components/spatial_component.inl.h"

#endif  // VW_ECS_COMPONENTS_SPATIAL_COMPONENT_H
