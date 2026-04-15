#pragma once

#ifndef VW_GFX_ENTITY_GUARD_GROUP_INL_H
#define VW_GFX_ENTITY_GUARD_GROUP_INL_H

#include "entity_guard.inl.h"

namespace vw::gfx {

template <typename WD>
entity_guard_group<WD>::entity_guard_group(
    context_type& ctx, uint32 count
)
    : context_(&ctx), entities_(ctx.batch_create_entities(count)), archetype_{} {}

template <typename WD>
entity_guard_group<WD>::~entity_guard_group() {
    cleanup_();
}

template <typename WD>
entity_guard_group<WD>::entity_guard_group(
    entity_guard_group&& other
) noexcept
    : context_(other.context_),
      entities_(std::move(other.entities_)),
      archetype_(other.archetype_) {
    other.context_   = nullptr;
    other.archetype_ = {};
}

template <typename WD>
auto entity_guard_group<WD>::operator=(
    entity_guard_group&& other
) noexcept -> entity_guard_group& {
    if (this != &other) {
        cleanup_();
        context_   = other.context_;
        entities_  = std::move(other.entities_);
        archetype_ = other.archetype_;
        other.context_   = nullptr;
        other.archetype_ = {};
    }
    return *this;
}

template <typename WD>
template <typename C>
auto entity_guard_group<WD>::with(
    C&& value
) -> entity_guard_group& {
    for (auto ent : entities_) {
        context_->template add_component<C>(ent, C{value});
    }
    archetype_.template add<C>();
    return *this;
}

template <typename WD>
template <typename C>
auto entity_guard_group<WD>::without() -> entity_guard_group& {
    for (auto ent : entities_) {
        context_->template remove_component<C>(ent);
    }
    archetype_.template remove<C>();
    return *this;
}

template <typename WD>
auto entity_guard_group<WD>::size() const -> uint32 {
    return static_cast<uint32>(entities_.size());
}

template <typename WD>
auto entity_guard_group<WD>::operator[](
    uint32 index
) const -> entity {
    return entities_[index];
}

template <typename WD>
auto entity_guard_group<WD>::entities() const -> const std::vector<entity>& {
    return entities_;
}

template <typename WD>
auto entity_guard_group<WD>::get_archetype() const -> entity_archetype_type {
    return archetype_;
}

template <typename WD>
auto entity_guard_group<WD>::begin() const -> std::vector<entity>::const_iterator {
    return entities_.begin();
}

template <typename WD>
auto entity_guard_group<WD>::end() const -> std::vector<entity>::const_iterator {
    return entities_.end();
}

template <typename WD>
void entity_guard_group<WD>::cleanup_() noexcept {
    if (context_ == nullptr || entities_.empty()) {
        return;
    }
    using components = typename WD::components;
    detail::for_each_tuple_type<components>([&]<typename C, std::size_t I>() noexcept {
        if (archetype_.template has<C>()) {
            for (auto ent : entities_) {
                context_->template remove_component<C>(ent);
            }
        }
    });
    context_->batch_destroy_entities(entities_);
    entities_.clear();
}

}  // namespace vw::gfx

#endif  // VW_GFX_ENTITY_GUARD_GROUP_INL_H
