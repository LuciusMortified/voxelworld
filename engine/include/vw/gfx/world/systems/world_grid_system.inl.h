#pragma once

#ifndef VW_GFX_WORLD_SYSTEMS_WORLD_GRID_SYSTEM_INL_H
#define VW_GFX_WORLD_SYSTEMS_WORLD_GRID_SYSTEM_INL_H

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
void world_grid_system<WC, Cs...>::update() {
    if (!world_grid_) {
        return;
    }

    world_grid_->process_completed();

    auto& requested = registry_->template requested<world_view_component>();
    if (requested.empty()) {
        return;
    }

    bool regions_dirty = false;
    for (auto ent : requested) {
        if (process_dirty_entity(ent)) {
            regions_dirty = true;
        }
        registry_->template notify_changed<world_view_component>(ent);
    }

    if (regions_dirty) {
        pending_active_regions_.clear();

        for (auto ent : requested) {
            if (!registry_->template has<world_view_component>(ent) ||
                !registry_->template has<transform_component>(ent)) {
                continue;
            }

            const auto& wv = registry_->template get<world_view_component>(ent);
            auto cc = wv.get_chunk_coord();
            auto dist = static_cast<int32>(wv.get_view_distance());

            for (int32 dx = -dist; dx <= dist; ++dx) {
                for (int32 dz = -dist; dz <= dist; ++dz) {
                    auto rid = world_grid_->get_region_id(cc.x + dx, cc.z + dz);
                    pending_active_regions_.insert(rid);
                }
            }
        }

        for (auto rid : pending_active_regions_) {
            if (!active_regions_.contains(rid)) {
                world_grid_->request_region(rid);
            }
        }

        for (auto rid : active_regions_) {
            if (!pending_active_regions_.contains(rid)) {
                world_grid_->unload_region(rid);
            }
        }

        std::swap(active_regions_, pending_active_regions_);
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

    if (wv.chunk_coord_ == new_chunk_coord) {
        return false;
    }

    wv.chunk_coord_ = new_chunk_coord;
    return true;
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

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_SYSTEMS_WORLD_GRID_SYSTEM_INL_H
