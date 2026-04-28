#pragma once

#ifndef VW_ECS_WORLD_INL_H
#define VW_ECS_WORLD_INL_H

#include <tuple>

#include "vw/ecs/system_trait.h"
#include "vw/ecs/systems/spatial_system.h"

namespace vw::ecs {

namespace detail {

template <typename Tuple, typename W, std::size_t... Is>
auto make_system_tuple_impl(W& w, std::index_sequence<Is...>) -> Tuple {
    return Tuple{std::tuple_element_t<Is, Tuple>(w)...};
}

template <typename Tuple, typename W>
auto make_system_tuple(W& w) -> Tuple {
    return make_system_tuple_impl<Tuple>(
        w, std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}

}  // namespace detail

template <typename WD>
world<WD>::world()
    : registry_{}
    , systems_{detail::make_system_tuple<systems>(*this)}
    , resources_{}
{}

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
auto world<WD>::has(
    entity ent
) const -> bool {
    return registry_.template has<T>(ent);
}

template <typename WD>
template <typename T>
auto world<WD>::get(
    entity ent
) -> T& {
    return registry_.template get<T>(ent);
}

template <typename WD>
template <typename T>
auto world<WD>::get(
    entity ent
) const -> const T& {
    return registry_.template get<T>(ent);
}

template <typename WD>
template <typename... Cs>
auto world<WD>::view() -> component_view<registry_type, Cs...> {
    return registry_.template view<Cs...>();
}

template <typename WD>
auto world<WD>::registry() -> registry_type& {
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
auto world<WD>::create_entity_() -> entity {
    return registry_.create();
}

template <typename WD>
auto world<WD>::batch_create_entities_(
    uint32 count
) -> std::vector<entity> {
    return registry_.batch_create(count);
}

template <typename WD>
template <typename T>
void world<WD>::add_component_(
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
void world<WD>::remove_component_(
    entity ent
) noexcept {
    std::apply([&](auto&... systems) {
        (detail::invoke_on_remove<T>(systems, ent), ...);
    }, systems_);
    registry_.template remove<T>(ent);
}

template <typename WD>
template <typename T>
void world<WD>::batch_add_component_(
    const std::vector<entity>& entities, const T& value
) {
    registry_.template batch_add<T>(entities, value);
    std::apply([&](auto&... systems) {
        for (auto ent : entities) {
            (detail::invoke_on_add<T>(systems, ent), ...);
        }
    }, systems_);
}

template <typename WD>
template <typename T>
void world<WD>::batch_remove_component_(
    const std::vector<entity>& entities
) noexcept {
    std::apply([&](auto&... systems) {
        for (auto ent : entities) {
            (detail::invoke_on_remove<T>(systems, ent), ...);
        }
    }, systems_);
    registry_.template batch_remove<T>(entities);
}

template <typename WD>
void world<WD>::destroy(
    entity ent
) noexcept {
    registry_.for_each_component(ent, [&]<typename C>() noexcept {
        this->template remove_component_<C>(ent);
    });
    registry_.destroy(ent);
}

template <typename WD>
void world<WD>::batch_destroy(
    const std::vector<entity>& entities
) noexcept {
    for (auto ent : entities) {
        registry_.for_each_component(ent, [&]<typename C>() noexcept {
            this->template remove_component_<C>(ent);
        });
    }
    registry_.batch_destroy(entities);
}

template <typename WD>
auto world<WD>::create() -> modifier {
    return modifier(*this, create_entity_());
}

template <typename WD>
auto world<WD>::modify(
    entity ent
) -> modifier {
    return modifier(*this, ent);
}

template <typename WD>
auto world<WD>::batch_create(
    uint32 count
) -> batch_modifier {
    return batch_modifier(*this, batch_create_entities_(count));
}

template <typename WD>
auto world<WD>::batch_modify(
    std::vector<entity> entities
) -> batch_modifier {
    return batch_modifier(*this, std::move(entities));
}

template <typename WD>
world<WD>::modifier::modifier(
    world& w, entity ent
)
    : world_(&w), ent_(ent) {}

template <typename WD>
template <typename C>
auto world<WD>::modifier::with(
    C&& value
) -> modifier& {
    world_->template add_component_<C>(ent_, std::forward<C>(value));
    return *this;
}

template <typename WD>
template <typename C>
auto world<WD>::modifier::without() -> modifier& {
    world_->template remove_component_<C>(ent_);
    return *this;
}

template <typename WD>
auto world<WD>::modifier::get_entity() const -> entity {
    return ent_;
}

template <typename WD>
world<WD>::batch_modifier::batch_modifier(
    world& w, std::vector<entity> entities
)
    : world_(&w), entities_(std::move(entities)) {}

template <typename WD>
template <typename C>
auto world<WD>::batch_modifier::with(
    const C& value
) -> batch_modifier& {
    world_->template batch_add_component_<C>(entities_, value);
    return *this;
}

template <typename WD>
template <typename C>
auto world<WD>::batch_modifier::without() -> batch_modifier& {
    world_->template batch_remove_component_<C>(entities_);
    return *this;
}

template <typename WD>
auto world<WD>::batch_modifier::get_entities() const -> const std::vector<entity>& {
    return entities_;
}

template <typename WD>
auto world<WD>::batch_modifier::release_entities() -> std::vector<entity> {
    return std::move(entities_);
}

}  // namespace vw::ecs

#endif  // VW_ECS_WORLD_INL_H
