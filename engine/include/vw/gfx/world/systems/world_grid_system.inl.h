#pragma once

#ifndef VW_GFX_WORLD_SYSTEMS_WORLD_GRID_SYSTEM_INL_H
#define VW_GFX_WORLD_SYSTEMS_WORLD_GRID_SYSTEM_INL_H

#include <chrono>

#include "vw/gfx/world/components/transform_component.h"

namespace vw::gfx {

template <typename WC, typename... Cs>
world_grid_system<WC, Cs...>::world_grid_system(
    registry_type& registry
)
    : registry_(&registry) {}

template <typename WC, typename... Cs>
void world_grid_system<WC, Cs...>::set_world_grid(
    std::shared_ptr<world_grid<WC>> grid
) {
    world_grid_ = std::move(grid);
}

template <typename WC, typename... Cs>
auto world_grid_system<WC, Cs...>::get_world_grid() const -> std::shared_ptr<world_grid<WC>> {
    return world_grid_;
}

template <typename WC, typename... Cs>
auto world_grid_system<WC, Cs...>::get_stats() const -> const world_grid_system_stats& {
    return stats_;
}

template <typename WC, typename... Cs>
void world_grid_system<WC, Cs...>::update() {
    if (!world_grid_) {
        return;
    }

    using clock = std::chrono::high_resolution_clock;
    auto ms = [](auto a, auto b) -> float32 {
        return std::chrono::duration<float32>(b - a).count() * 1000.0f;
    };

    auto t0 = clock::now();
    world_grid_->process_completed();
    auto t1 = clock::now();

    static constexpr int32 max_requests_per_frame = 8;
    int32 requests = 0;
    while (!pending_requests_.empty() && requests < max_requests_per_frame) {
        auto coord = pending_requests_.back();
        pending_requests_.pop_back();
        if (world_grid_->request_chunk(coord)) {
            ++requests;
        }
    }
    auto t2 = clock::now();

    stats_.process_completed_ms  = ms(t0, t1);
    stats_.request_chunks_ms     = ms(t1, t2);
    stats_.active_count          = static_cast<uint32>(active_chunks_.size());
    stats_.pending_count         = world_grid_->get_pending_chunk_count();
    stats_.loaded_count          = world_grid_->get_loaded_chunk_count();
    stats_.deferred_remesh_count = world_grid_->get_deferred_remesh_count();
    stats_.rebuild_active_ms     = 0.0f;
    stats_.unload_ms             = 0.0f;

    auto& requested = registry_->template requested<world_view_component>();
    if (requested.empty()) {
        return;
    }

    vec3i camera_chunk{};
    bool chunks_dirty = false;
    for (auto ent : requested) {
        if (process_dirty_entity(ent)) {
            chunks_dirty = true;
        }
        registry_->template notify_changed<world_view_component>(ent);
    }

    if (chunks_dirty) {
        auto t3 = clock::now();

        pending_active_chunks_.clear();

        for (auto ent : requested) {
            if (!registry_->template has<world_view_component>(ent) ||
                !registry_->template has<transform_component>(ent)) {
                continue;
            }

            const auto& wv = registry_->template get<world_view_component>(ent);
            camera_chunk = wv.get_chunk_coord();
            auto dist = static_cast<int32>(wv.get_view_distance());

            auto& gen = world_grid_->get_generator();
            for (int32 dx = -dist; dx <= dist; ++dx) {
                for (int32 dz = -dist; dz <= dist; ++dz) {
                    int32 cx = camera_chunk.x + dx;
                    int32 cz = camera_chunk.z + dz;
                    auto yr = gen.get_chunk_y_range(cx, cz);
                    for (int32 cy = yr.min_y; cy <= yr.max_y; ++cy) {
                        pending_active_chunks_.insert({cx, cy, cz});
                    }
                }
            }
        }
        auto t4 = clock::now();

        for (const auto& coord : active_chunks_) {
            if (!pending_active_chunks_.contains(coord)) {
                world_grid_->unload_chunk(coord);
            }
        }
        auto t5 = clock::now();

        std::swap(active_chunks_, pending_active_chunks_);
        rebuild_pending_requests(camera_chunk);

        stats_.rebuild_active_ms = ms(t3, t4);
        stats_.unload_ms         = ms(t4, t5);
        stats_.active_count      = static_cast<uint32>(active_chunks_.size());
    }

    registry_->template clear_requested<world_view_component>();
}

template <typename WC, typename... Cs>
auto world_grid_system<WC, Cs...>::process_dirty_entity(
    entity ent
) -> bool {
    if (!registry_->template has<world_view_component>(ent) ||
        !registry_->template has<transform_component>(ent)) {
        return false;
    }

    auto& wv = registry_->template get<world_view_component>(ent);
    const auto& tc = registry_->template get<transform_component>(ent);
    auto pos = tc.get_position();

    auto new_chunk_coord = world_grid_->world_to_chunk_coord({
        static_cast<int32>(pos.x),
        static_cast<int32>(pos.y),
        static_cast<int32>(pos.z)
    });

    bool changed = wv.dirty_ || wv.chunk_coord_ != new_chunk_coord;
    wv.chunk_coord_ = new_chunk_coord;
    wv.dirty_ = false;
    return changed;
}

template <typename WC, typename... Cs>
world_grid_system<WC, Cs...>::view_modifier::view_modifier(
    world_grid_system* system, entity ent
)
    : system_(system), entity_(ent) {}

template <typename WC, typename... Cs>
auto world_grid_system<WC, Cs...>::modify_view(
    entity ent
) -> view_modifier {
    return view_modifier(this, ent);
}

template <typename WC, typename... Cs>
auto world_grid_system<WC, Cs...>::view_modifier::set_view_distance(
    uint32 distance
) -> view_modifier& {
    if (!system_->registry_->template has<world_view_component>(entity_)) {
        return *this;
    }

    auto& wv = system_->registry_->template get<world_view_component>(entity_);
    wv.view_distance_ = distance;
    system_->registry_->template request_update<world_view_component>(entity_);

    return *this;
}

template <typename WC, typename... Cs>
void world_grid_system<WC, Cs...>::rebuild_pending_requests(
    vec3i camera_chunk
) {
    pending_requests_.clear();

    for (const auto& coord : active_chunks_) {
        if (!world_grid_->has_chunk(coord) && !world_grid_->is_pending(coord)) {
            pending_requests_.push_back(coord);
        }
    }

    std::sort(pending_requests_.begin(), pending_requests_.end(),
        [&camera_chunk](const vec3i& a, const vec3i& b) {
            auto da = a - camera_chunk;
            auto db = b - camera_chunk;
            auto dist_a = da.x * da.x + da.y * da.y + da.z * da.z;
            auto dist_b = db.x * db.x + db.y * db.y + db.z * db.z;
            return dist_a > dist_b;
        });
}

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_SYSTEMS_WORLD_GRID_SYSTEM_INL_H
