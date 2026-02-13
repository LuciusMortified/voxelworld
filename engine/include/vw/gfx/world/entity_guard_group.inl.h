#pragma once

#ifndef VW_GFX_ENTITY_GUARD_GROUP_INL_H
#define VW_GFX_ENTITY_GUARD_GROUP_INL_H

#include "entity_guard.inl.h"
#include "world.h"

namespace vw::gfx {

template <typename WC>
entity_guard_group<WC>::entity_guard_group(
    world_type& world, uint32 count
)
    : world_(&world), entities_(world.batch_create_entities(count)), archetype_{} {}

template <typename WC>
entity_guard_group<WC>::~entity_guard_group() {
    cleanup_();
}

template <typename WC>
entity_guard_group<WC>::entity_guard_group(
    entity_guard_group&& other
) noexcept
    : world_(other.world_),
      entities_(std::move(other.entities_)),
      archetype_(other.archetype_) {
    other.world_     = nullptr;
    other.archetype_ = {};
}

template <typename WC>
auto entity_guard_group<WC>::operator=(
    entity_guard_group&& other
) noexcept -> entity_guard_group& {
    if (this != &other) {
        cleanup_();
        world_     = other.world_;
        entities_  = std::move(other.entities_);
        archetype_ = other.archetype_;
        other.world_     = nullptr;
        other.archetype_ = {};
    }
    return *this;
}

template <typename WC>
template <typename C>
auto entity_guard_group<WC>::with(
    C&& value
) -> entity_guard_group& {
    for (auto ent : entities_) {
        world_->template add_component<C>(ent, C{value});
    }
    archetype_.template add<C>();
    return *this;
}

template <typename WC>
template <typename C>
auto entity_guard_group<WC>::without() -> entity_guard_group& {
    for (auto ent : entities_) {
        world_->template remove_component<C>(ent);
    }
    archetype_.template remove<C>();
    return *this;
}

template <typename WC>
auto entity_guard_group<WC>::size() const -> uint32 {
    return static_cast<uint32>(entities_.size());
}

template <typename WC>
auto entity_guard_group<WC>::operator[](
    uint32 index
) const -> entity {
    return entities_[index];
}

template <typename WC>
auto entity_guard_group<WC>::entities() const -> const std::vector<entity>& {
    return entities_;
}

template <typename WC>
auto entity_guard_group<WC>::get_archetype() const -> entity_archetype_type {
    return archetype_;
}

template <typename WC>
auto entity_guard_group<WC>::begin() const -> std::vector<entity>::const_iterator {
    return entities_.begin();
}

template <typename WC>
auto entity_guard_group<WC>::end() const -> std::vector<entity>::const_iterator {
    return entities_.end();
}

template <typename WC>
void entity_guard_group<WC>::cleanup_() noexcept {
    if (world_ == nullptr || entities_.empty()) {
        return;
    }
    detail::for_each_tuple_type<WC>([&]<typename C, std::size_t I>() noexcept {
        if (archetype_.template has<C>()) {
            for (auto ent : entities_) {
                world_->template remove_component<C>(ent);
            }
        }
    });
    world_->batch_destroy_entities(entities_);
    entities_.clear();
}

}  // namespace vw::gfx

#endif  // VW_GFX_ENTITY_GUARD_GROUP_INL_H
