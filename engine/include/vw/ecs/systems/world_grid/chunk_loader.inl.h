#pragma once

#ifndef VW_ECS_SYSTEMS_WORLD_GRID_CHUNK_LOADER_INL_H
#define VW_ECS_SYSTEMS_WORLD_GRID_CHUNK_LOADER_INL_H

#include <algorithm>

#include "vw/asset/model/model.h"

namespace vw::ecs {

inline chunk_loader::chunk_loader(
    std::unique_ptr<terrain_generator> generator
)
    : generator_(std::move(generator)) {
    auto count = std::min(std::thread::hardware_concurrency(), 4u);
    if (count == 0) {
        count = 1;
    }
    for (uint32 i = 0; i < count; ++i) {
        gen_threads_.emplace_back(&chunk_loader::gen_thread_function_, this);
    }
}

inline chunk_loader::~chunk_loader() {
    {
        std::scoped_lock lock(gen_mutex_);
        gen_running_ = false;
    }
    gen_cv_.notify_all();

    for (auto& t : gen_threads_) {
        if (t.joinable()) {
            t.join();
        }
    }
}

inline auto chunk_loader::request(
    vec2i coord
) -> bool {
    if (pending_columns_.contains(coord)) {
        return false;
    }
    pending_columns_.insert(coord);

    {
        std::scoped_lock lock(gen_mutex_);
        gen_queue_.push({coord});
    }
    gen_cv_.notify_one();
    return true;
}

inline auto chunk_loader::try_pop_completed() -> std::unique_ptr<gen_column> {
    std::unique_ptr<gen_column> col;
    {
        std::scoped_lock lock(completed_mutex_);
        if (completed_queue_.empty()) {
            return nullptr;
        }
        col = std::move(completed_queue_.front());
        completed_queue_.pop();
    }
    pending_columns_.erase(col->get_coord());
    return col;
}

inline auto chunk_loader::is_pending(
    vec2i coord
) const -> bool {
    return pending_columns_.contains(coord);
}

inline auto chunk_loader::pending_count() const -> uint32 {
    return static_cast<uint32>(pending_columns_.size());
}

inline void chunk_loader::gen_thread_function_() {
    while (true) {
        gen_task task{};

        {
            std::unique_lock lock(gen_mutex_);
            gen_cv_.wait(lock, [this] -> bool { return !gen_queue_.empty() || !gen_running_; });

            if (!gen_running_ && gen_queue_.empty()) {
                break;
            }

            if (!gen_queue_.empty()) {
                task = gen_queue_.front();
                gen_queue_.pop();
            } else {
                continue;
            }
        }

        auto col = std::make_unique<gen_column>(task.coord.x, task.coord.y);

        terrain_context ctx{
            .cx           = task.coord.x,
            .cz           = task.coord.y,
            .create_chunk = [&col](int32 y) -> chunk_data& {
                return col->create_chunk(y, chunk_data{});
            }
        };

        generator_->generate(ctx);
        col->set_phase(column_phase::terrain);

        for (auto& [y, cd] : col->get_all_chunk_data()) {
            cd.chunk_model->compute_own_boundaries();
        }

        {
            std::scoped_lock lock(completed_mutex_);
            completed_queue_.push(std::move(col));
        }
    }
}

}  // namespace vw::ecs

#endif  // VW_ECS_SYSTEMS_WORLD_GRID_CHUNK_LOADER_INL_H
