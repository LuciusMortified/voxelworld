#pragma once

#ifndef VW_GFX_WORLD_GRID_INL_H
#define VW_GFX_WORLD_GRID_INL_H

#include "vw/core/timing.h"
#include "vw/gfx/world/world.h"
#include "vw/log/logger.h"

namespace vw::gfx {

namespace {
constexpr log::log_category lc_wg_{"world_grid"};
}

template <typename WC>
world_grid<WC>::world_grid(
    world_type& world, std::unique_ptr<terrain_generator> generator, int32 voxel_scale
)
    : world_(&world), voxel_scale_(voxel_scale), generator_(std::move(generator)) {
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
    return it->second->get_voxel(lc / voxel_scale_);
}

template <typename WC>
auto world_grid<WC>::get_surface_y(
    int32 wx, int32 wz
) const -> std::optional<int32> {
    constexpr int32 s = chunk<WC>::size;

    auto floor_div = [](int32 a, int32 b) -> int32 { return a >= 0 ? a / b : (a - b + 1) / b; };
    int32 cx       = floor_div(wx, s);
    int32 cz       = floor_div(wz, s);
    vec2i col_coord{cx, cz};

    auto col_it = column_chunks_.find(col_coord);
    if (col_it == column_chunks_.end() || col_it->second.empty()) {
        return std::nullopt;
    }

    const auto& y_levels = col_it->second;

    int32 local_x = ((wx % s) + s) % s;
    int32 local_z = ((wz % s) + s) % s;

    for (auto it = y_levels.rbegin(); it != y_levels.rend(); ++it) {
        int32 cy = *it;
        vec3i chunk_coord{cx, cy, cz};
        auto chunk_it = chunks_.find(chunk_coord);
        if (chunk_it == chunks_.end()) {
            continue;
        }

        for (int32 local_y = s - 1; local_y >= 0; --local_y) {
            if (!chunk_it->second->get_voxel(local_x, local_y, local_z).is_empty()) {
                return (cy * s) + local_y;
            }
        }
    }

    return std::nullopt;
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
    it->second->set_voxel(lc / voxel_scale_, v);
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
auto world_grid<WC>::has_column(
    vec2i coord
) const -> bool {
    return column_chunks_.contains(coord);
}

template <typename WC>
auto world_grid<WC>::get_loaded_column_count() const -> uint32 {
    return static_cast<uint32>(column_chunks_.size());
}

template <typename WC>
auto world_grid<WC>::get_loaded_chunk_count() const -> uint32 {
    return static_cast<uint32>(chunks_.size());
}

template <typename WC>
auto world_grid<WC>::get_pending_column_count() const -> uint32 {
    return static_cast<uint32>(pending_columns_.size());
}

template <typename WC>
auto world_grid<WC>::get_deferred_remesh_count() const -> uint32 {
    return static_cast<uint32>(deferred_remeshes_.size());
}

template <typename WC>
auto world_grid<WC>::is_column_pending(
    vec2i coord
) const -> bool {
    return pending_columns_.contains(coord);
}

template <typename WC>
auto world_grid<WC>::voxel_scale() const -> int32 {
    return voxel_scale_;
}

template <typename WC>
auto world_grid<WC>::get_terrain_generator() const -> terrain_generator& {
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
    return {((world_pos.x % s) + s) % s, ((world_pos.y % s) + s) % s, ((world_pos.z % s) + s) % s};
}

template <typename WC>
auto world_grid<WC>::chunk_to_world_coord(
    vec3i chunk_coord
) const -> vec3i {
    const int32 s = chunk<WC>::size * voxel_scale_;
    return {chunk_coord.x * s, chunk_coord.y * s, chunk_coord.z * s};
}

template <typename WC>
auto world_grid<WC>::request_column(
    vec2i coord
) -> bool {
    if (column_chunks_.contains(coord) || pending_columns_.contains(coord)) {
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

template <typename WC>
void world_grid<WC>::unload_column(
    vec2i coord
) {
    auto col_it = column_chunks_.find(coord);
    if (col_it != column_chunks_.end()) {
        for (int32 y : col_it->second) {
            vec3i chunk_coord{coord.x, y, coord.y};
            chunks_.erase(chunk_coord);
        }
        column_chunks_.erase(col_it);
    }
    pending_columns_.erase(coord);
}

template <typename WC>
auto world_grid<WC>::get_completed_stats() const -> const completed_stats& {
    return completed_stats_;
}

template <typename WC>
void world_grid<WC>::process_completed() {
    static constexpr int32 max_columns_per_frame = 4;
    static constexpr vec3i neighbor_offsets[6]   = {
        {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}
    };

    float32 boundary_from_total = 0.0f;
    float32 chunk_create_total  = 0.0f;
    float32 boundary_to_total   = 0.0f;
    int32 processed_columns     = 0;
    int32 processed_chunks      = 0;

    while (processed_columns < max_columns_per_frame) {
        std::unique_ptr<gen_column> col;
        {
            std::scoped_lock lock(completed_mutex_);
            if (completed_queue_.empty()) {
                break;
            }
            col = std::move(completed_queue_.front());
            completed_queue_.pop();
        }

        auto col_coord = col->get_coord();
        if (!pending_columns_.contains(col_coord)) {
            continue;
        }

        pending_columns_.erase(col_coord);

        std::vector<int32> y_levels;

        for (auto& [y, cd] : col->get_all_chunk_data()) {
            boundary_from_total += measure_ms([&] -> auto {
                for (int fd = 0; fd < 6; ++fd) {
                    auto* neighbor = get_chunk(cd.coord + neighbor_offsets[fd]);
                    if (neighbor) {
                        cd.chunk_model->set_boundary_slice(fd, *neighbor->get_model());
                    }
                }
            });

            chunk_create_total += measure_ms([&] -> auto {
                chunks_.emplace(
                    cd.coord,
                    std::make_unique<chunk<WC>>(
                        *world_, cd.coord, std::move(cd.chunk_model), voxel_scale_
                    )
                );
            });

            boundary_to_total += measure_ms([&] -> auto {
                auto* created = get_chunk(cd.coord);
                for (int fd = 0; fd < 6; ++fd) {
                    auto* neighbor = get_chunk(cd.coord + neighbor_offsets[fd]);
                    if (neighbor) {
                        int opposite_fd = fd ^ 1;
                        neighbor->get_model()->set_boundary_slice(
                            opposite_fd, *created->get_model()
                        );
                        deferred_remeshes_.push({cd.coord + neighbor_offsets[fd], opposite_fd});
                    }
                }
            });

            y_levels.push_back(y);
            ++processed_chunks;
        }

        std::sort(y_levels.begin(), y_levels.end());
        column_chunks_[col_coord] = std::move(y_levels);
        ++processed_columns;
    }

    completed_stats_.deferred_ms        = measure_ms([&] -> auto { process_deferred_remeshes(); });
    completed_stats_.boundary_from_ms   = boundary_from_total;
    completed_stats_.chunk_create_ms    = chunk_create_total;
    completed_stats_.boundary_to_ms     = boundary_to_total;
    completed_stats_.chunks_processed   = static_cast<uint32>(processed_chunks);
    completed_stats_.remeshes_processed = 0;
}

template <typename WC>
void world_grid<WC>::process_deferred_remeshes() {
    static constexpr int32 max_remeshes_per_frame = 4;
    int32 processed                               = 0;

    while (processed < max_remeshes_per_frame && !deferred_remeshes_.empty()) {
        auto [coord, fd] = deferred_remeshes_.front();
        deferred_remeshes_.pop();

        auto* chunk_ptr = get_chunk(coord);
        if (!chunk_ptr)
            continue;

        auto model = chunk_ptr->get_model();
        model->invalidate();
        world_->get_model_system().modify(chunk_ptr->get_entity()).set_model(model);
        ++processed;
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

        auto col = std::make_unique<gen_column>(task.coord.x, task.coord.y);

        terrain_context ctx{
            .cx = task.coord.x, .cz = task.coord.y, .create_chunk = [&col](int32 y) -> chunk_data& {
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

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_GRID_INL_H
