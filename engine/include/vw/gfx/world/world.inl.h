#pragma once

#ifndef VW_GFX_WORLD_WORLD_INL_H
#define VW_GFX_WORLD_WORLD_INL_H

#include "vw/core/timing.h"

namespace vw::gfx {

template <typename Cs>
world<Cs>::world(
    vulkan_context& context, const block_registry& registry
)
    : mesh_pool_{context, registry}
    , context_{registry_, &mesh_pool_, nullptr}
    , spatial_system_(context_)
    , transform_system_(context_, hierarchy_system_)
    , hierarchy_system_(context_, transform_system_)
    , model_system_(context_)
    , light_system_(context_)
    , socket_system_(context_, hierarchy_system_, transform_system_)
    , animation_system_(context_, transform_system_, animation_clip_registry_)
    , character_controller_system_(context_, transform_system_)
    , physics_system_(context_, transform_system_)
    , world_grid_system_(context_) {}

template <typename Cs>
void world<Cs>::update(
    float32 delta_time
) {
    registry_.clear_changed();
    update_stats_.character_controller_ms = measure_ms([&] { character_controller_system_.update(delta_time); });
    update_stats_.physics_ms              = measure_ms([&] { physics_system_.update(delta_time); });
    update_stats_.transform_ms            = measure_ms([&] { transform_system_.update(); });
    update_stats_.model_ms                = measure_ms([&] { model_system_.update(); });
    update_stats_.spatial_ms              = measure_ms([&] { spatial_system_.update(); });
    update_stats_.light_ms                = measure_ms([&] { light_system_.update(); });
    update_stats_.world_grid_ms           = measure_ms([&] { world_grid_system_.update(); });
    update_stats_.animation_ms            = measure_ms([&] { animation_system_.update(delta_time); });
}

template <typename Cs>
auto world<Cs>::get_update_stats() const -> const world_update_stats& {
    return update_stats_;
}

template <typename Cs>
template <typename T>
auto world<Cs>::has_component(
    entity ent
) const -> bool {
    return registry_.template has<T>(ent);
}

template <typename Cs>
template <typename T>
auto world<Cs>::get_component(
    entity ent
) -> T& {
    return registry_.template get<T>(ent);
}

template <typename WC>
auto world<WC>::get_mesh_pool() const -> const mesh_pool& {
    return mesh_pool_;
}

template <typename WC>
auto world<WC>::get_mesh_pool() -> mesh_pool& {
    return mesh_pool_;
}

template <typename C>
template <typename... Cs>
auto world<C>::view_components() -> component_view<registry_type, Cs...> {
    return registry_.template view<Cs...>();
}

template <typename C>
auto world<C>::get_hierarchy_system() -> hierarchy_system_type& {
    return hierarchy_system_;
}

template <typename C>
auto world<C>::get_transform_system() -> transform_system_type& {
    return transform_system_;
}

template <typename C>
auto world<C>::get_model_registry() -> model_registry& {
    return model_registry_;
}

template <typename C>
auto world<C>::get_model_system() -> model_system_type& {
    return model_system_;
}

template <typename C>
auto world<C>::get_spatial_system() -> spatial_system_type& {
    return spatial_system_;
}

template <typename C>
auto world<C>::get_light_system() -> light_system_type& {
    return light_system_;
}

template <typename C>
auto world<C>::get_socket_system() -> socket_system_type& {
    return socket_system_;
}

template <typename C>
auto world<C>::get_animation_system() -> animation_system_type& {
    return animation_system_;
}

template <typename C>
auto world<C>::get_animation_clip_registry() -> animation_clip_registry& {
    return animation_clip_registry_;
}

template <typename C>
void world<C>::set_world_grid(std::shared_ptr<world_grid<C>> grid) {
    context_.world_grid = std::move(grid);
}

template <typename C>
auto world<C>::get_world_grid() const -> std::shared_ptr<world_grid<C>> {
    return context_.world_grid;
}

template <typename C>
auto world<C>::get_world_grid_system() -> world_grid_system_type& {
    return world_grid_system_;
}

template <typename C>
auto world<C>::get_character_controller_system() -> character_controller_system_type& {
    return character_controller_system_;
}

template <typename C>
auto world<C>::get_physics_system() -> physics_system_type& {
    return physics_system_;
}

template <typename C>
auto world<C>::get_registry() -> registry_type& {
    return registry_;
}

template <typename C>
template <typename T>
auto world<C>::changed() -> std::unordered_set<entity>& {
    return registry_.template changed<T>();
}

template <typename C>
auto world<C>::destroyed() const -> const std::vector<entity>& {
    return registry_.destroyed();
}

template <typename WC>
auto world<WC>::voxel_ray_cast(
    const ray& r, std::unordered_set<entity>& candidates
) const -> std::optional<voxel_ray_hit> {
    return spatial_system_.voxel_ray_cast(r, candidates);
}

template <typename Cs>
template <typename T>
void world<Cs>::add_component(
    entity ent, T&& value
) {
    registry_.add(ent, std::forward<T>(value));

    if constexpr (std::same_as<T, transform_component>) {
        registry_.template request_change<transform_component>(ent);
    }
    if constexpr (std::same_as<T, model_component>) {
        registry_.template request_change<model_component>(ent);
    }
    if constexpr (std::same_as<T, spatial_component>) {
        registry_.template request_change<transform_component>(ent);
    }
    if constexpr (std::same_as<T, light_component>) {
        registry_.template request_change<light_component>(ent);
    }
}

template <typename Cs>
template <typename T>
void world<Cs>::remove_component(
    entity ent
) noexcept {
    if constexpr (std::same_as<T, hierarchy_component>) {
        hierarchy_system_.cleanup(ent);
    }
    if constexpr (std::same_as<T, spatial_component>) {
        spatial_system_.cleanup(ent);
    }
    if constexpr (std::same_as<T, socket_component>) {
        socket_system_.cleanup(ent);
    }
    registry_.template remove<T>(ent);
}

template <typename Cs>
auto world<Cs>::create_entity() -> entity {
    return registry_.create();
}

template <typename C>
void world<C>::destroy_entity(
    entity ent
) noexcept {
    registry_.destroy(ent);
}

template <typename C>
auto world<C>::batch_create_entities(
    uint32 count
) -> std::vector<entity> {
    return registry_.batch_create(count);
}

template <typename C>
void world<C>::batch_destroy_entities(
    const std::vector<entity>& entities
) noexcept {
    registry_.batch_destroy(entities);
}

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_WORLD_INL_H
