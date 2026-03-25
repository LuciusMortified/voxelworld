#pragma once

#ifndef VW_GFX_WORLD_GRID_H
#define VW_GFX_WORLD_GRID_H

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "vw/core.h"
#include "vw/gfx/world_grid/chunk.h"
#include "vw/gfx/world_grid/world_grid_generator.h"

namespace vw::gfx {

template <typename WC>
class world;

template <typename, typename...>
class world_grid_system;

template <typename WC = base_world_components>
class world_grid {
    using world_type = world<WC>;
    using chunk_type = chunk<WC>;

    template <typename, typename...>
    friend class world_grid_system;

public:
    explicit world_grid(world_type& world, std::unique_ptr<world_grid_generator> generator,
                        int32 voxel_scale = 8);
    ~world_grid();

    world_grid(const world_grid&) = delete;
    auto operator=(const world_grid&) -> world_grid& = delete;
    world_grid(world_grid&&) = delete;
    auto operator=(world_grid&&) -> world_grid& = delete;

    [[nodiscard]] auto get_voxel(vec3i world_pos) const -> voxel;
    void set_voxel(vec3i world_pos, const voxel& v);

    [[nodiscard]] auto has_chunk(vec3i chunk_coord) const -> bool;
    [[nodiscard]] auto get_chunk(vec3i chunk_coord) -> chunk<WC>*;

    [[nodiscard]] auto get_loaded_chunk_count() const -> uint32;
    [[nodiscard]] auto get_pending_chunk_count() const -> uint32;
    [[nodiscard]] auto get_deferred_remesh_count() const -> uint32;
    [[nodiscard]] auto is_pending(vec3i chunk_coord) const -> bool;

    struct completed_stats {
        float32 boundary_from_ms  = 0.0f;
        float32 chunk_create_ms   = 0.0f;
        float32 boundary_to_ms    = 0.0f;
        float32 deferred_ms       = 0.0f;
        uint32 chunks_processed   = 0;
        uint32 remeshes_processed = 0;
    };

    [[nodiscard]] auto get_completed_stats() const -> const completed_stats&;

    [[nodiscard]] auto voxel_scale() const -> int32;
    [[nodiscard]] auto get_generator() -> world_grid_generator&;

    auto world_to_chunk_coord(vec3i world_pos) const -> vec3i;
    auto world_to_local_coord(vec3i world_pos) const -> vec3i;
    auto chunk_to_world_coord(vec3i chunk_coord) const -> vec3i;

private:
    auto request_chunk(vec3i coord) -> bool;
    void unload_chunk(vec3i coord);
    void process_completed();
    void gen_thread_function();

    world_type* world_;
    int32 voxel_scale_{1};
    std::unordered_map<vec3i, std::unique_ptr<chunk_type>> chunks_;
    std::unique_ptr<world_grid_generator> generator_;

    struct gen_task {
        vec3i coord;
    };

    std::vector<std::thread> gen_threads_;
    std::queue<gen_task> gen_queue_;
    std::queue<chunk_data> completed_queue_;
    std::mutex gen_mutex_;
    std::mutex completed_mutex_;
    std::condition_variable gen_cv_;
    bool gen_running_ = true;
    std::unordered_set<vec3i> pending_chunks_;

    struct deferred_remesh {
        vec3i chunk_coord;
        int face_direction;
    };
    std::queue<deferred_remesh> deferred_remeshes_;
    void process_deferred_remeshes();
    completed_stats completed_stats_;
};

}  // namespace vw::gfx

#include "vw/gfx/world_grid/world_grid.inl.h"

#endif  // VW_GFX_WORLD_GRID_H
