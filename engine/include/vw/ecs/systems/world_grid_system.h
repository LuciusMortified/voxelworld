#pragma once

#ifndef VW_ECS_SYSTEMS_WORLD_GRID_SYSTEM_H
#define VW_ECS_SYSTEMS_WORLD_GRID_SYSTEM_H

#include <memory>
#include <queue>
#include <unordered_set>
#include <vector>

#include "vw/ecs/components/world_view_component.h"
#include "vw/ecs/entity_registry.h"
#include "vw/ecs/system_trait.h"
#include "vw/ecs/systems/world_grid/chunk_loader.h"

namespace vw::ecs {

class world_grid;

class world;

struct world_grid_system_stats {
    float32 integrate_ms         = 0.0f;
    float32 boundary_from_ms     = 0.0f;
    float32 chunk_create_ms      = 0.0f;
    float32 boundary_to_ms       = 0.0f;
    float32 deferred_remesh_ms   = 0.0f;
    float32 request_columns_ms   = 0.0f;
    float32 rebuild_active_ms    = 0.0f;
    float32 unload_ms            = 0.0f;
    uint32 active_count          = 0;
    uint32 pending_count         = 0;
    uint32 loaded_count          = 0;
    uint32 deferred_remesh_count = 0;
};

class world_grid_system {
public:
    using registry_type = registry;
    using world_type    = world;
    using grid_type     = world_grid;

    explicit world_grid_system(world_type& w);
    ~world_grid_system();

    world_grid_system(const world_grid_system&)                    = delete;
    auto operator=(const world_grid_system&) -> world_grid_system& = delete;
    world_grid_system(world_grid_system&&) noexcept;
    auto operator=(world_grid_system&&) noexcept -> world_grid_system&;

    void set_grid(std::unique_ptr<grid_type> grid);
    void set_loader(std::unique_ptr<chunk_loader> loader);

    [[nodiscard]] auto grid() -> grid_type*;
    [[nodiscard]] auto grid() const -> const grid_type*;
    [[nodiscard]] auto loader() -> chunk_loader*;
    [[nodiscard]] auto loader() const -> const chunk_loader*;
    [[nodiscard]] auto has_grid() const -> bool;
    [[nodiscard]] auto has_loader() const -> bool;

    void update(float32 dt);
    void shutdown();

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
    struct deferred_remesh {
        vec3i chunk_coord;
        int32 face_direction;
    };

    auto process_dirty_entity_(entity ent) -> bool;
    auto process_dirty_entities_() -> bool;
    void integrate_completed_columns_();
    void process_deferred_remeshes_();
    void dispatch_column_requests_();
    void update_grid_stats_();
    auto rebuild_active_set_() -> vec2i;
    void unload_inactive_columns_();
    void rebuild_pending_requests_(vec2i camera_column);
    void clear_grid_transient_state_();
    void clear_loader_transient_state_();

    world_type* world_;
    std::unique_ptr<grid_type> grid_;
    std::unique_ptr<chunk_loader> loader_;
    std::unordered_set<vec2i> active_columns_;
    std::unordered_set<vec2i> pending_active_columns_;
    std::vector<vec2i> pending_requests_;
    std::queue<deferred_remesh> deferred_remeshes_;
    world_grid_system_stats stats_;
};

}  // namespace vw::ecs

#endif  // VW_ECS_SYSTEMS_WORLD_GRID_SYSTEM_H
