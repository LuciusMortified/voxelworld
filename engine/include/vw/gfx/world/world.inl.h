#pragma once

#ifndef VW_GFX_WORLD_WORLD_INL_H
#define VW_GFX_WORLD_WORLD_INL_H

#include <tuple>

namespace vw::gfx {

namespace detail {

template <typename Tuple, typename Ctx, std::size_t... Is>
auto make_system_tuple_impl(Ctx& ctx, std::index_sequence<Is...>) -> Tuple {
    return Tuple{std::tuple_element_t<Is, Tuple>(ctx)...};
}

template <typename Tuple, typename Ctx>
auto make_system_tuple(Ctx& ctx) -> Tuple {
    return make_system_tuple_impl<Tuple>(
        ctx, std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}

}  // namespace detail

template <typename WD>
world<WD>::~world() {
    context_.world_grid_.reset();
}

template <typename WD>
world<WD>::world()
    : context_{registry_}
    , systems_{detail::make_system_tuple<systems_tuple>(context_)}
    , resources_{}
{
    context_.systems_ = &systems_;
    std::apply([this](auto&... r) { (context_.register_resource_(&r), ...); }, resources_);
}

template <typename WD>
void world<WD>::update(
    float32 delta_time
) {
    std::apply([delta_time](auto&... s) {
        (s.update(delta_time), ...);
    }, systems_);
}

template <typename WD>
void world<WD>::clear_changed() {
    registry_.clear_changed();
}

template <typename WD>
template <typename T>
auto world<WD>::has_component(
    entity ent
) const -> bool {
    return registry_.template has<T>(ent);
}

template <typename WD>
template <typename T>
auto world<WD>::get_component(
    entity ent
) -> T& {
    return registry_.template get<T>(ent);
}

template <typename WD>
template <typename T>
auto world<WD>::get_component(
    entity ent
) const -> const T& {
    return registry_.template get<T>(ent);
}

template <typename WD>
template <typename... Cs>
auto world<WD>::view_components() -> component_view<registry_type, Cs...> {
    return registry_.template view<Cs...>();
}

template <typename WD>
void world<WD>::set_world_grid(
    std::shared_ptr<world_grid<WD>> grid
) {
    context_.world_grid_ = std::move(grid);
}

template <typename WD>
auto world<WD>::get_world_grid() const -> std::shared_ptr<world_grid<WD>> {
    return context_.world_grid_;
}

template <typename WD>
auto world<WD>::get_registry() -> registry_type& {
    return registry_;
}

template <typename WD>
template <typename T>
auto world<WD>::changed() -> std::unordered_set<entity>& {
    return registry_.template changed<T>();
}

template <typename WD>
auto world<WD>::destroyed() const -> const std::vector<entity>& {
    return registry_.destroyed();
}

template <typename WD>
auto world<WD>::voxel_ray_cast(
    const ray& r, std::vector<entity>& candidates
) const -> std::optional<voxel_ray_hit> {
    return std::get<spatial_system<WD>>(systems_).voxel_ray_cast(r, candidates);
}

template <typename WD>
template <typename T>
void world<WD>::add_component(
    entity ent, T&& value
) {
    registry_.add(ent, std::forward<T>(value));

    using C = std::remove_cvref_t<T>;
    std::apply([&](auto&... systems) {
        (detail::invoke_on_add<C>(systems, ent), ...);
    }, systems_);
}

template <typename WD>
template <typename T>
void world<WD>::remove_component(
    entity ent
) noexcept {
    std::apply([&](auto&... systems) {
        (detail::invoke_on_remove<T>(systems, ent), ...);
    }, systems_);
    registry_.template remove<T>(ent);
}

template <typename WD>
auto world<WD>::create_entity() -> entity {
    return registry_.create();
}

template <typename WD>
void world<WD>::destroy_entity(
    entity ent
) noexcept {
    registry_.destroy(ent);
}

template <typename WD>
auto world<WD>::batch_create_entities(
    uint32 count
) -> std::vector<entity> {
    return registry_.batch_create(count);
}

template <typename WD>
void world<WD>::batch_destroy_entities(
    const std::vector<entity>& entities
) noexcept {
    registry_.batch_destroy(entities);
}

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_WORLD_INL_H
