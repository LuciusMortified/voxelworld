#pragma once

#ifndef VW_GFX_ENTITY_GUARD_INL_H
#define VW_GFX_ENTITY_GUARD_INL_H

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
    world_type& world, entity ent, entity_archetype_type archetype
)
    : world_(&world), ent_(ent), archetype_(archetype) {}

template <typename WC>
entity_guard<WC>::~entity_guard() {
    detail::for_each_tuple_type<WC>([&]<typename C, std::size_t I>() noexcept {
        if (archetype_.template has<C>()) {
            world_->template remove_component<C>(ent_);
        }
    });
    world_->destroy_entity(ent_);
}

template <typename WC>
auto entity_guard<WC>::get_entity() const -> entity {
    return ent_;
}

template <typename WC>
auto entity_guard<WC>::get_archetype() const -> entity_archetype_type {
    return archetype_;
}

}  // namespace vw::gfx

#endif  // VW_GFX_ENTITY_GUARD_INL_H
