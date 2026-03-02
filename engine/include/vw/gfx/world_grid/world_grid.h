#pragma once

#ifndef VW_GFX_WORLD_GRID_H
#define VW_GFX_WORLD_GRID_H

#include <algorithm>
#include <condition_variable>
#include <deque>
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
    explicit world_grid(world_type& world, std::unique_ptr<world_grid_generator> generator);
    ~world_grid();

    world_grid(const world_grid&) = delete;
    auto operator=(const world_grid&) -> world_grid& = delete;
    world_grid(world_grid&&) = delete;
    auto operator=(world_grid&&) -> world_grid& = delete;

    [[nodiscard]] auto get_voxel(vec3i world_pos) const -> voxel;
    void set_voxel(vec3i world_pos, const voxel& v);

    [[nodiscard]] auto has_chunk(vec3i chunk_coord) const -> bool;
    [[nodiscard]] auto get_chunk(vec3i chunk_coord) -> chunk<WC>*;

    [[nodiscard]] auto get_region_id(int32 cx, int32 cz) const -> region_id;
    [[nodiscard]] auto is_region_loaded(region_id id) const -> bool;
    [[nodiscard]] auto is_region_pending(region_id id) const -> bool;
    [[nodiscard]] auto get_region_meta(region_id id) const -> const region_meta*;

    [[nodiscard]] auto get_loaded_chunk_count() const -> uint32;
    [[nodiscard]] auto get_loaded_region_count() const -> uint32;
    [[nodiscard]] auto get_pending_region_count() const -> uint32;
    [[nodiscard]] auto get_pending_chunk_count() const -> uint32;

    static auto world_to_chunk_coord(vec3i world_pos) -> vec3i;
    static auto world_to_local_coord(vec3i world_pos) -> vec3i;
    static auto chunk_to_world_coord(vec3i chunk_coord) -> vec3i;

private:
    void request_region(region_id id);
    void unload_region(region_id id);
    void process_completed();
    void gen_thread_function();

    world_type* world_;
    std::unordered_map<vec3i, std::unique_ptr<chunk_type>> chunks_;
    std::unordered_map<region_id, std::vector<vec3i>> region_chunks_;
    std::unordered_map<vec2i, region_id> column_region_cache_;
    std::unordered_map<region_id, region_meta> region_meta_cache_;
    std::unordered_set<region_id> loaded_regions_;
    std::unique_ptr<world_grid_generator> generator_;

    struct gen_task {
        region_id id;
    };

    static constexpr uint32 chunks_per_frame_ = 16;

    std::vector<std::thread> gen_threads_;
    std::queue<gen_task> gen_queue_;
    std::queue<region_gen_result> completed_queue_;
    std::mutex gen_mutex_;
    std::mutex completed_mutex_;
    std::condition_variable gen_cv_;
    bool gen_running_ = true;
    std::unordered_set<region_id> pending_regions_;

    std::deque<chunk_data> pending_chunks_;
    std::unordered_map<region_id, uint32> region_remaining_chunks_;
};

}  // namespace vw::gfx

#include "vw/gfx/world_grid/world_grid.inl.h"

#endif  // VW_GFX_WORLD_GRID_H
