#pragma once

#ifndef VW_GFX_WORLD_GRID_INL_H
#define VW_GFX_WORLD_GRID_INL_H

#include "vw/gfx/world/world.h"

namespace vw::gfx {

template <typename WC>
world_grid<WC>::world_grid(
    world_type& world, std::unique_ptr<world_grid_generator> generator, int32 voxel_scale
)
    : world_(&world)
    , voxel_scale_(voxel_scale)
    , generator_(std::move(generator)) {
    generator_->set_identity_pool(world.get_model_registry().get_identity_pool());
    auto count = std::min(std::thread::hardware_concurrency(), 4u);
    if (count == 0) {
        count = 1;
    }
    for (uint32 i = 0; i < count; ++i) {
        gen_threads_.emplace_back(&world_grid<WC>::gen_thread_function, this);
    }
}

template <typename WC>
world_grid<WC>::~world_grid() {
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

template <typename WC>
auto world_grid<WC>::get_voxel(
    vec3i world_pos
) const -> voxel {
    auto cc = world_to_chunk_coord(world_pos);
    auto it = chunks_.find(cc);
    if (it == chunks_.end()) {
        return empty_voxel;
    }
    auto lc = world_to_local_coord(world_pos);
    return it->second->get_voxel(lc);
}

template <typename WC>
void world_grid<WC>::set_voxel(
    vec3i world_pos, const voxel& v
) {
    auto cc = world_to_chunk_coord(world_pos);
    auto it = chunks_.find(cc);
    if (it == chunks_.end()) {
        return;
    }
    auto lc = world_to_local_coord(world_pos);
    it->second->set_voxel(lc, v);
}

template <typename WC>
auto world_grid<WC>::has_chunk(
    vec3i chunk_coord
) const -> bool {
    return chunks_.contains(chunk_coord);
}

template <typename WC>
auto world_grid<WC>::get_chunk(
    vec3i chunk_coord
) -> chunk<WC>* {
    auto it = chunks_.find(chunk_coord);
    return it != chunks_.end() ? it->second.get() : nullptr;
}

template <typename WC>
auto world_grid<WC>::get_loaded_chunk_count() const -> uint32 {
    return static_cast<uint32>(chunks_.size());
}

template <typename WC>
auto world_grid<WC>::get_pending_chunk_count() const -> uint32 {
    return static_cast<uint32>(pending_chunks_.size());
}

template <typename WC>
auto world_grid<WC>::voxel_scale() const -> int32 {
    return voxel_scale_;
}

template <typename WC>
auto world_grid<WC>::get_generator() -> world_grid_generator& {
    return *generator_;
}

template <typename WC>
auto world_grid<WC>::world_to_chunk_coord(
    vec3i world_pos
) const -> vec3i {
    const int32 s = chunk<WC>::size * voxel_scale_;
    return {
        world_pos.x >= 0 ? world_pos.x / s : (world_pos.x - s + 1) / s,
        world_pos.y >= 0 ? world_pos.y / s : (world_pos.y - s + 1) / s,
        world_pos.z >= 0 ? world_pos.z / s : (world_pos.z - s + 1) / s
    };
}

template <typename WC>
auto world_grid<WC>::world_to_local_coord(
    vec3i world_pos
) const -> vec3i {
    const int32 s = chunk<WC>::size * voxel_scale_;
    return {
        ((world_pos.x % s) + s) % s,
        ((world_pos.y % s) + s) % s,
        ((world_pos.z % s) + s) % s
    };
}

template <typename WC>
auto world_grid<WC>::chunk_to_world_coord(
    vec3i chunk_coord
) const -> vec3i {
    const int32 s = chunk<WC>::size * voxel_scale_;
    return {chunk_coord.x * s, chunk_coord.y * s, chunk_coord.z * s};
}

template <typename WC>
void world_grid<WC>::request_chunk(
    vec3i coord
) {
    if (chunks_.contains(coord) || pending_chunks_.contains(coord)) {
        return;
    }

    pending_chunks_.insert(coord);

    {
        std::scoped_lock lock(gen_mutex_);
        gen_queue_.push({coord});
    }
    gen_cv_.notify_one();
}

template <typename WC>
void world_grid<WC>::unload_chunk(
    vec3i coord
) {
    chunks_.erase(coord);
    pending_chunks_.erase(coord);
}

template <typename WC>
void world_grid<WC>::process_completed() {
    std::queue<chunk_data> local_queue;
    {
        std::scoped_lock lock(completed_mutex_);
        std::swap(local_queue, completed_queue_);
    }

    while (!local_queue.empty()) {
        auto cd = std::move(local_queue.front());
        local_queue.pop();

        if (!pending_chunks_.contains(cd.coord)) {
            continue;
        }

        pending_chunks_.erase(cd.coord);
        chunks_.emplace(
            cd.coord,
            std::make_unique<chunk<WC>>(*world_, cd.coord, std::move(cd.chunk_model), voxel_scale_)
        );
    }
}

template <typename WC>
void world_grid<WC>::gen_thread_function() {
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

        auto result = generator_->generate_chunk(task.coord);

        {
            std::scoped_lock lock(completed_mutex_);
            completed_queue_.push(std::move(result));
        }
    }
}

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_GRID_INL_H
