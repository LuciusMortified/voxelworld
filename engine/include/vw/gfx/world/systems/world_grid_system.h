#pragma once

#ifndef VW_GFX_WORLD_SYSTEMS_WORLD_GRID_SYSTEM_H
#define VW_GFX_WORLD_SYSTEMS_WORLD_GRID_SYSTEM_H

#include <algorithm>
#include <memory>
#include <unordered_set>
#include <vector>

#include "vw/gfx/world/components/world_view_component.h"
#include "vw/gfx/world/registry.h"
#include "vw/gfx/world_grid/world_grid.h"

namespace vw::gfx {

struct world_grid_system_stats {
    float32 process_completed_ms = 0.0f;
    float32 request_chunks_ms    = 0.0f;
    float32 rebuild_active_ms    = 0.0f;
    float32 unload_ms            = 0.0f;
    uint32 active_count          = 0;
    uint32 pending_count         = 0;
    uint32 loaded_count          = 0;
    uint32 deferred_remesh_count = 0;
};

template <typename WC, typename... Cs>
class world_grid_system {
public:
    using registry_type = registry<Cs...>;

    explicit world_grid_system(registry_type& registry);

    void set_world_grid(std::shared_ptr<world_grid<WC>> grid);
    [[nodiscard]] auto get_world_grid() const -> std::shared_ptr<world_grid<WC>>;

    void update();

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

    void rebuild_pending_requests(vec3i camera_chunk);

    registry_type* registry_;
    std::shared_ptr<world_grid<WC>> world_grid_;
    std::unordered_set<vec3i> active_chunks_;
    std::unordered_set<vec3i> pending_active_chunks_;
    std::vector<vec3i> pending_requests_;
    world_grid_system_stats stats_;
};

template <typename WC, typename... Cs>
struct world_grid_system_from_tuple;

template <typename WC, typename... Cs>
struct world_grid_system_from_tuple<WC, std::tuple<Cs...>> {
    using type = world_grid_system<WC, Cs...>;
};

}  // namespace vw::gfx

#include "vw/gfx/world/systems/world_grid_system.inl.h"

#endif  // VW_GFX_WORLD_SYSTEMS_WORLD_GRID_SYSTEM_H
