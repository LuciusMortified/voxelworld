#pragma once

#ifndef VW_GFX_WORLD_WORLD_INL_H
#define VW_GFX_WORLD_WORLD_INL_H


namespace vw::gfx {

template <typename Cs>
world<Cs>::world(
    vulkan_context& context
)
    : mesh_pool_{context},
      spatial_system_(registry_),
      transform_system_(registry_, spatial_system_, hierarchy_system_),
      hierarchy_system_(registry_, transform_system_),
      model_system_(registry_, mesh_pool_, spatial_system_),
      light_system_(registry_),
      socket_system_(registry_, hierarchy_system_, transform_system_),
      animation_system_(registry_, transform_system_, animation_clip_registry_) {}

template <typename Cs>
void world<Cs>::update(float32 delta_time) {
    transform_system_.update();
    model_system_.update();
    spatial_system_.update();
    animation_system_.update(delta_time);
}

template <typename Cs>
auto world<Cs>::create_entity() -> entity {
    return registry_.create();
}

template <typename Cs>
template <typename T>
void world<Cs>::add_component(
    entity ent, T&& value
) {
    registry_.add(ent, std::forward<T>(value));

    if constexpr (std::same_as<T, transform_component>) {
        transform_system_.mark_dirty(ent);
        transform_system_.mark_render_dirty(ent);
    }
    if constexpr (std::same_as<T, model_component>) {
        model_system_.mark_dirty(ent);
        model_system_.mark_render_dirty(ent);
    }
    if constexpr (std::same_as<T, spatial_component>) {
        spatial_system_.mark_dirty(ent);
    }
    if constexpr (std::same_as<T, light_component>) {
        light_system_.mark_render_dirty(ent);
    }
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

template <typename Cs>
template <typename T>
void world<Cs>::remove_component(
    entity ent
) noexcept {
    if constexpr (std::same_as<T, hierarchy_component>) {
        hierarchy_system_.cleanup(ent);
    }
    if constexpr (std::same_as<T, transform_component>) {
        transform_system_.mark_render_dirty(ent);
    }
    if constexpr (std::same_as<T, model_component>) {
        model_system_.mark_render_dirty(ent);
    }
    if constexpr (std::same_as<T, spatial_component>) {
        spatial_system_.cleanup(ent);
    }
    if constexpr (std::same_as<T, light_component>) {
        light_system_.mark_render_dirty(ent);
    }
    if constexpr (std::same_as<T, socket_component>) {
        socket_system_.cleanup(ent);
    }
    registry_.template remove<T>(ent);
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

template <typename WC>
auto world<WC>::get_mesh_pool() const -> const mesh_pool& {
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

template <typename WC>
auto world<WC>::voxel_ray_cast(
    const ray& r,
    std::unordered_set<entity>& candidates
) const -> std::optional<voxel_ray_hit> {
    return spatial_system_.voxel_ray_cast(r, candidates);
}

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_WORLD_INL_H
