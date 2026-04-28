#pragma once

#ifndef VW_GFX_WORLD_SYSTEMS_WORLD_GRID_SYSTEM_INL_H
#define VW_GFX_WORLD_SYSTEMS_WORLD_GRID_SYSTEM_INL_H

#include "vw/core/timing.h"
#include "vw/gfx/world/components/transform_component.h"
#include "vw/gfx/world/world.h"
#include "vw/gfx/world_grid/world_grid.h"

namespace vw::gfx {

template <typename WD>
world_grid_system<WD>::world_grid_system(
    world_type& w
)
    : world_(&w) {}

template <typename WD>
world_grid_system<WD>::~world_grid_system() = default;

template <typename WD>
world_grid_system<WD>::world_grid_system(world_grid_system&&) noexcept = default;

template <typename WD>
auto world_grid_system<WD>::operator=(world_grid_system&&) noexcept -> world_grid_system& = default;

template <typename WD>
void world_grid_system<WD>::set_grid(
    std::unique_ptr<grid_type> grid
) {
    grid_ = std::move(grid);
}

template <typename WD>
auto world_grid_system<WD>::grid() -> grid_type* {
    return grid_.get();
}

template <typename WD>
auto world_grid_system<WD>::grid() const -> const grid_type* {
    return grid_.get();
}

template <typename WD>
auto world_grid_system<WD>::has_grid() const -> bool {
    return grid_ != nullptr;
}

template <typename WD>
auto world_grid_system<WD>::get_stats() const -> const world_grid_system_stats& {
    return stats_;
}

template <typename WD>
void world_grid_system<WD>::update(float32 /*dt*/) {
    if (!grid_) {
        return;
    }

    auto& reg = world_->registry();
    stats_.process_completed_ms = measure_ms([&] { grid_->process_completed(); });
    stats_.request_columns_ms   = measure_ms([&] { dispatch_column_requests(); });
    update_grid_stats();

    if (reg.template requested<world_view_component>().empty()) {
        return;
    }

    if (process_dirty_entities()) {
        vec2i camera_column{};
        stats_.rebuild_active_ms = measure_ms([&] { camera_column = rebuild_active_set(); });
        stats_.unload_ms         = measure_ms([&] { unload_inactive_columns(); });
        std::swap(active_columns_, pending_active_columns_);
        rebuild_pending_requests(camera_column);
        stats_.active_count = static_cast<uint32>(active_columns_.size());
    }

    reg.template clear_requested<world_view_component>();
}

template <typename WD>
void world_grid_system<WD>::dispatch_column_requests() {
    static constexpr int32 max_requests_per_frame = 8;
    int32 requests = 0;
    while (!pending_requests_.empty() && requests < max_requests_per_frame) {
        auto coord = pending_requests_.back();
        pending_requests_.pop_back();
        if (grid_->request_column(coord)) {
            ++requests;
        }
    }
}

template <typename WD>
void world_grid_system<WD>::update_grid_stats() {
    stats_.active_count          = static_cast<uint32>(active_columns_.size());
    stats_.pending_count         = grid_->get_pending_column_count();
    stats_.loaded_count          = grid_->get_loaded_chunk_count();
    stats_.deferred_remesh_count = grid_->get_deferred_remesh_count();
    stats_.rebuild_active_ms     = 0.0f;
    stats_.unload_ms             = 0.0f;
}

template <typename WD>
auto world_grid_system<WD>::process_dirty_entities() -> bool {
    auto& reg = world_->registry();
    bool chunks_dirty = false;
    for (auto ent : reg.template requested<world_view_component>()) {
        if (process_dirty_entity(ent)) {
            chunks_dirty = true;
        }
        reg.template notify_changed<world_view_component>(ent);
    }
    return chunks_dirty;
}

template <typename WD>
auto world_grid_system<WD>::rebuild_active_set() -> vec2i {
    auto& reg = world_->registry();
    pending_active_columns_.clear();
    vec2i camera_column{};

    for (auto ent : reg.template requested<world_view_component>()) {
        if (!reg.template has<world_view_component>(ent) ||
            !reg.template has<transform_component>(ent)) {
            continue;
        }

        const auto& wv = reg.template get<world_view_component>(ent);
        auto chunk_coord = wv.get_chunk_coord();
        camera_column = {chunk_coord.x, chunk_coord.z};
        const auto dist = static_cast<int32>(wv.get_view_distance());

        for (int32 dx = -dist; dx <= dist; ++dx) {
            for (int32 dz = -dist; dz <= dist; ++dz) {
                int32 cx = camera_column.x + dx;
                int32 cz = camera_column.y + dz;
                pending_active_columns_.insert({cx, cz});
            }
        }
    }

    return camera_column;
}

template <typename WD>
void world_grid_system<WD>::unload_inactive_columns() {
    for (const auto& coord : active_columns_) {
        if (!pending_active_columns_.contains(coord)) {
            grid_->unload_column(coord);
        }
    }
}

template <typename WD>
auto world_grid_system<WD>::process_dirty_entity(
    entity ent
) -> bool {
    auto& reg = world_->registry();
    if (!reg.template has<world_view_component>(ent) ||
        !reg.template has<transform_component>(ent)) {
        return false;
    }

    auto& wv = reg.template get<world_view_component>(ent);
    const auto& tc = reg.template get<transform_component>(ent);
    auto pos = tc.get_position();

    auto new_chunk_coord = grid_->world_to_chunk_coord({
        static_cast<int32>(pos.x),
        static_cast<int32>(pos.y),
        static_cast<int32>(pos.z)
    });

    bool changed = wv.dirty_ || wv.chunk_coord_ != new_chunk_coord;
    wv.chunk_coord_ = new_chunk_coord;
    wv.dirty_ = false;
    return changed;
}

template <typename WD>
world_grid_system<WD>::view_modifier::view_modifier(
    world_grid_system* system, entity ent
)
    : system_(system), entity_(ent) {}

template <typename WD>
auto world_grid_system<WD>::modify_view(
    entity ent
) -> view_modifier {
    return view_modifier(this, ent);
}

template <typename WD>
auto world_grid_system<WD>::view_modifier::set_view_distance(
    uint32 distance
) -> view_modifier& {
    auto& reg = system_->world_->registry();
    if (!reg.template has<world_view_component>(entity_)) {
        return *this;
    }

    auto& wv = reg.template get<world_view_component>(entity_);
    wv.view_distance_ = distance;
    reg.template request_update<world_view_component>(entity_);

    return *this;
}

template <typename WD>
void world_grid_system<WD>::rebuild_pending_requests(
    vec2i camera_column
) {
    pending_requests_.clear();

    for (const auto& coord : active_columns_) {
        if (!grid_->has_column(coord) && !grid_->is_column_pending(coord)) {
            pending_requests_.push_back(coord);
        }
    }

    std::sort(pending_requests_.begin(), pending_requests_.end(),
        [&camera_column](const vec2i& a, const vec2i& b) {
            auto da = a - camera_column;
            auto db = b - camera_column;
            auto dist_a = da.x * da.x + da.y * da.y;
            auto dist_b = db.x * db.x + db.y * db.y;
            return dist_a > dist_b;
        });
}

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_SYSTEMS_WORLD_GRID_SYSTEM_INL_H
