#pragma once

#ifndef VW_ECS_SYSTEMS_WORLD_GRID_SYSTEM_H
#define VW_ECS_SYSTEMS_WORLD_GRID_SYSTEM_H

#include <algorithm>
#include <memory>
#include <unordered_set>
#include <vector>

#include "vw/ecs/components/world_view_component.h"
#include "vw/ecs/entity_registry.h"
#include "vw/ecs/system_trait.h"

namespace vw::ecs {

template <typename>
class world_grid;

template <typename>
class world;

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
    using components    = typename WD::components;
    using registry_type = typename entity_registry_from_tuple<components>::type;
    using world_type    = world<WD>;
    using grid_type     = world_grid<WD>;

    explicit world_grid_system(world_type& w);
    ~world_grid_system();

    world_grid_system(const world_grid_system&)                    = delete;
    auto operator=(const world_grid_system&) -> world_grid_system& = delete;
    world_grid_system(world_grid_system&&) noexcept;
    auto operator=(world_grid_system&&) noexcept -> world_grid_system&;

    void set_grid(std::unique_ptr<grid_type> grid);
    [[nodiscard]] auto grid() -> grid_type*;
    [[nodiscard]] auto grid() const -> const grid_type*;
    [[nodiscard]] auto has_grid() const -> bool;

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

    world_type* world_;
    std::unique_ptr<grid_type> grid_;
    std::unordered_set<vec2i> active_columns_;
    std::unordered_set<vec2i> pending_active_columns_;
    std::vector<vec2i> pending_requests_;
    world_grid_system_stats stats_;
};

}  // namespace vw::ecs

template <>
struct vw::ecs::system_trait<vw::ecs::world_grid_system> {
    using components = std::tuple<vw::ecs::world_view_component>;
    using resources  = std::tuple<>;
};

#include "vw/ecs/systems/world_grid_system.inl.h"

#endif  // VW_ECS_SYSTEMS_WORLD_GRID_SYSTEM_H
