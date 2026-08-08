#pragma once

#ifndef VW_ECS_COMPONENTS_WORLD_VIEW_COMPONENT_H
#define VW_ECS_COMPONENTS_WORLD_VIEW_COMPONENT_H

#include "vw/core.h"

namespace vw::ecs {

class world_grid_system;

struct world_view_component final {
    [[nodiscard]] auto get_chunk_coord() const -> vec3i { return chunk_coord_; }
    [[nodiscard]] auto get_view_distance() const -> uint32 { return view_distance_; }

private:
    vec3i chunk_coord_{0, 0, 0};
    uint32 view_distance_{10};
    bool dirty_ = true;

        friend class world_grid_system;
};

}  // namespace vw::ecs

#endif  // VW_ECS_COMPONENTS_WORLD_VIEW_COMPONENT_H
