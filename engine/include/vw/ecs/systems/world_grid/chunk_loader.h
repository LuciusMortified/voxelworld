#pragma once

#ifndef VW_ECS_SYSTEMS_WORLD_GRID_CHUNK_LOADER_H
#define VW_ECS_SYSTEMS_WORLD_GRID_CHUNK_LOADER_H

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_set>
#include <vector>

#include "vw/core.h"
#include "vw/ecs/systems/world_grid/gen_column.h"
#include "vw/ecs/systems/world_grid/terrain_generator.h"

namespace vw::ecs {

class chunk_loader {
public:
    explicit chunk_loader(std::unique_ptr<terrain_generator> generator);
    ~chunk_loader();

    chunk_loader(const chunk_loader&)                    = delete;
    auto operator=(const chunk_loader&) -> chunk_loader& = delete;
    chunk_loader(chunk_loader&&)                         = delete;
    auto operator=(chunk_loader&&) -> chunk_loader&      = delete;

    auto request(vec2i coord) -> bool;
    [[nodiscard]] auto try_pop_completed() -> std::unique_ptr<gen_column>;
    [[nodiscard]] auto is_pending(vec2i coord) const -> bool;
    [[nodiscard]] auto pending_count() const -> uint32;

private:
    void gen_thread_function_();

    struct gen_task {
        vec2i coord;
    };

    std::unique_ptr<terrain_generator> generator_;
    std::vector<std::thread> gen_threads_;
    std::queue<gen_task> gen_queue_;
    std::queue<std::unique_ptr<gen_column>> completed_queue_;
    std::mutex gen_mutex_;
    mutable std::mutex completed_mutex_;
    std::condition_variable gen_cv_;
    bool gen_running_ = true;
    std::unordered_set<vec2i> pending_columns_;
};

}  // namespace vw::ecs

#include "vw/ecs/systems/world_grid/chunk_loader.inl.h"

#endif  // VW_ECS_SYSTEMS_WORLD_GRID_CHUNK_LOADER_H
