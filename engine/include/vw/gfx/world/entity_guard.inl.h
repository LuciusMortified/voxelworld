#pragma once

#ifndef VW_GFX_ENTITY_GUARD_INL_H
#define VW_GFX_ENTITY_GUARD_INL_H

#include "world.h"

namespace vw::gfx {

namespace detail {

template <typename Tuple, typename F, std::size_t... Is>
void for_each_tuple_type_impl(F&& f, std::index_sequence<Is...>) noexcept {
    (static_cast<void>(f.template operator()<std::tuple_element_t<Is, Tuple>, Is>()), ...);
}

template <typename Tuple, typename F>
void for_each_tuple_type(F&& f) noexcept {
    constexpr std::size_t N = std::tuple_size_v<Tuple>;
    for_each_tuple_type_impl<Tuple>(std::forward<F>(f), std::make_index_sequence<N>{});
}

}  // namespace detail

template <typename WC>
entity_guard<WC>::entity_guard(
    world_type& world
)
    : world_(&world), ent_(world.create_entity()), archetype_{} {}

template <typename WC>
entity_guard<WC>::entity_guard(
    world_type& world, entity ent, entity_archetype_type archetype
)
    : world_(&world), ent_(ent), archetype_(archetype) {}

template <typename WC>
entity_guard<WC>::~entity_guard() {
    cleanup_();
}

template <typename WC>
entity_guard<WC>::entity_guard(
    entity_guard&& other
) noexcept
    : world_(other.world_), ent_(other.ent_), archetype_(other.archetype_) {
    other.world_ = nullptr;
    other.ent_   = invalid_entity;
    other.archetype_ = {};
}

template <typename WC>
auto entity_guard<WC>::operator=(
    entity_guard&& other
) noexcept -> entity_guard& {
    if (this != &other) {
        cleanup_();
        world_     = other.world_;
        ent_       = other.ent_;
        archetype_ = other.archetype_;
        other.world_ = nullptr;
        other.ent_   = invalid_entity;
        other.archetype_ = {};
    }
    return *this;
}

template <typename WC>
template <typename C>
auto entity_guard<WC>::with(
    C&& value
) -> entity_guard& {
    world_->template add_component<C>(ent_, std::forward<C>(value));
    archetype_.template add<C>();
    return *this;
}

template <typename WC>
template <typename C>
auto entity_guard<WC>::without() -> entity_guard& {
    world_->template remove_component<C>(ent_);
    archetype_.template remove<C>();
    return *this;
}

template <typename WC>
auto entity_guard<WC>::get_entity() const -> entity {
    return ent_;
}

template <typename WC>
auto entity_guard<WC>::get_archetype() const -> entity_archetype_type {
    return archetype_;
}

template <typename WC>
auto entity_guard<WC>::is_valid() const -> bool {
    return world_ != nullptr && ent_.is_valid();
}

template <typename WC>
auto entity_guard<WC>::release() -> entity {
    auto ent = ent_;
    world_     = nullptr;
    ent_       = invalid_entity;
    archetype_ = {};
    return ent;
}

template <typename WC>
void entity_guard<WC>::update_archetype() {
    entity_archetype_type new_archetype;
    detail::for_each_tuple_type<WC>([&]<typename C, std::size_t I>() noexcept {
        if (world_->template has_component<C>(ent_)) {
            new_archetype.template add<C>();
        }
    });
    archetype_ = new_archetype;
}

template <typename WC>
void entity_guard<WC>::cleanup_() noexcept {
    if (world_ == nullptr || !ent_.is_valid()) {
        return;
    }
    detail::for_each_tuple_type<WC>([&]<typename C, std::size_t I>() noexcept {
        if (archetype_.template has<C>()) {
            world_->template remove_component<C>(ent_);
        }
    });
    world_->destroy_entity(ent_);
}

}  // namespace vw::gfx

#endif  // VW_GFX_ENTITY_GUARD_INL_H
