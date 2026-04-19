#pragma once

#ifndef VW_GFX_WORLD_SYSTEMS_WORLD_GRID_SYSTEM_H
#define VW_GFX_WORLD_SYSTEMS_WORLD_GRID_SYSTEM_H

#include <algorithm>
#include <memory>
#include <unordered_set>
#include <vector>

#include "vw/gfx/world/components/world_view_component.h"
#include "vw/gfx/world/entity_registry.h"
#include "vw/gfx/world/system_trait.h"
#include "vw/gfx/world/world_context.h"

namespace vw::gfx {

template <typename>
class world_grid;

struct world_grid_system_stats {
    float32 process_completed_ms = 0.0f;
    float32 request_columns_ms   = 0.0f;
    float32 rebuild_active_ms    = 0.0f;
    float32 unload_ms            = 0.0f;
    uint32 active_count          = 0;
    uint32 pending_count         = 0;
    uint32 loaded_count          = 0;
    uint32 deferred_remesh_count = 0;
};

template <typename WD>
class world_grid_system {
public:
    using components = typename WD::components;
    using registry_type = typename entity_registry_from_tuple<components>::type;
    using context_type = world_context<WD>;

    explicit world_grid_system(context_type& context);

    void update(float32 dt);

    [[nodiscard]] auto get_stats() const -> const world_grid_system_stats&;

    class view_modifier {
    public:
        auto set_view_distance(uint32 distance) -> view_modifier&;

    private:
        friend class world_grid_system;
        view_modifier(world_grid_system* system, entity ent);

        world_grid_system* system_;
        entity entity_;
    };

    auto modify_view(entity ent) -> view_modifier;

private:
    auto process_dirty_entity(entity ent) -> bool;
    void dispatch_column_requests();
    void update_grid_stats();
    auto process_dirty_entities() -> bool;
    auto rebuild_active_set() -> vec2i;
    void unload_inactive_columns();

    void rebuild_pending_requests(vec2i camera_column);

    context_type* context_;
    std::unordered_set<vec2i> active_columns_;
    std::unordered_set<vec2i> pending_active_columns_;
    std::vector<vec2i> pending_requests_;
    world_grid_system_stats stats_;
};

}  // namespace vw::gfx

template <>
struct vw::gfx::system_trait<vw::gfx::world_grid_system> {
    using components = std::tuple<vw::gfx::world_view_component>;
    using resources  = std::tuple<>;
};

#include "vw/gfx/world/systems/world_grid_system.inl.h"

#endif  // VW_GFX_WORLD_SYSTEMS_WORLD_GRID_SYSTEM_H
