#pragma once

#ifndef VW_GFX_ENTITY_GUARD_INL_H
#define VW_GFX_ENTITY_GUARD_INL_H

namespace vw::gfx {

namespace detail {

template <typename Tuple, typename F, std::size_t... Is>
void for_each_tuple_type_impl(
    F&& f, std::index_sequence<Is...>
) noexcept {
    (static_cast<void>(f.template operator()<std::tuple_element_t<Is, Tuple>, Is>()), ...);
}

template <typename Tuple, typename F>
void for_each_tuple_type(
    F&& f
) noexcept {
    constexpr std::size_t N = std::tuple_size_v<Tuple>;
    for_each_tuple_type_impl<Tuple>(std::forward<F>(f), std::make_index_sequence<N>{});
}

}  // namespace detail

template <typename WD>
entity_guard<WD>::entity_guard(
    context_type& ctx
)
    : context_(&ctx), ent_(ctx.create_entity()), archetype_{} {}

template <typename WD>
entity_guard<WD>::entity_guard(
    context_type& ctx, entity ent, entity_archetype_type archetype
)
    : context_(&ctx), ent_(ent), archetype_(archetype) {}

template <typename WD>
entity_guard<WD>::~entity_guard() {
    cleanup_();
}

template <typename WD>
entity_guard<WD>::entity_guard(
    entity_guard&& other
) noexcept
    : context_(other.context_), ent_(other.ent_), archetype_(other.archetype_) {
    other.context_   = nullptr;
    other.ent_       = invalid_entity;
    other.archetype_ = {};
}

template <typename WD>
auto entity_guard<WD>::operator=(
    entity_guard&& other
) noexcept -> entity_guard& {
    if (this != &other) {
        cleanup_();
        context_         = other.context_;
        ent_             = other.ent_;
        archetype_       = other.archetype_;
        other.context_   = nullptr;
        other.ent_       = invalid_entity;
        other.archetype_ = {};
    }
    return *this;
}

template <typename WD>
template <typename C>
auto entity_guard<WD>::with(
    C&& value
) -> entity_guard& {
    context_->template add_component<C>(ent_, std::forward<C>(value));
    archetype_.template add<C>();
    return *this;
}

template <typename WD>
template <typename C>
auto entity_guard<WD>::without() -> entity_guard& {
    context_->template remove_component<C>(ent_);
    archetype_.template remove<C>();
    return *this;
}

template <typename WD>
template <typename C>
auto entity_guard<WD>::has() const -> bool {
    return context_->template has_component<C>(ent_);
}

template <typename WD>
template <typename C>
auto entity_guard<WD>::get() const -> const C& {
    return context_->template get_component<C>(ent_);
}

template <typename WD>
auto entity_guard<WD>::get_entity() const -> entity {
    return ent_;
}

template <typename WD>
auto entity_guard<WD>::get_archetype() const -> entity_archetype_type {
    return archetype_;
}

template <typename WD>
auto entity_guard<WD>::is_valid() const -> bool {
    return context_ != nullptr && ent_.is_valid();
}

template <typename WD>
auto entity_guard<WD>::release() -> entity {
    auto ent   = ent_;
    context_   = nullptr;
    ent_       = invalid_entity;
    archetype_ = {};
    return ent;
}

template <typename WD>
void entity_guard<WD>::update_archetype() {
    using components = typename WD::components;
    entity_archetype_type new_archetype;
    detail::for_each_tuple_type<components>([&]<typename C, std::size_t I>() noexcept {
        if (context_->template has_component<C>(ent_)) {
            new_archetype.template add<C>();
        }
    });
    archetype_ = new_archetype;
}

template <typename WD>
auto entity_guard<WD>::operator*() const -> entity {
    return ent_;
}

template <typename WD>
void entity_guard<WD>::cleanup_() noexcept {
    if (context_ == nullptr || !ent_.is_valid()) {
        return;
    }
    using components = typename WD::components;
    detail::for_each_tuple_type<components>([&]<typename C, std::size_t I>() noexcept {
        if (archetype_.template has<C>()) {
            context_->template remove_component<C>(ent_);
        }
    });
    context_->destroy_entity(ent_);
}

}  // namespace vw::gfx

#endif  // VW_GFX_ENTITY_GUARD_INL_H
