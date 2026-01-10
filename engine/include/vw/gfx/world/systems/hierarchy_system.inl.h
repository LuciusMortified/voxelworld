#pragma once

#include "vw/gfx/world/entity.h"
#ifndef VW_GFX_HIERARCHY_SYSTEM_INL_H
#define VW_GFX_HIERARCHY_SYSTEM_INL_H

#include <stdexcept>

namespace vw::gfx {

template <typename... Cs>
hierarchy_system<Cs...>::hierarchy_system(
    registry_type& registry,
    transform_system_type& transform_sys
)
    : registry_{&registry}, transform_system_{&transform_sys} {}

template <typename... Cs>
hierarchy_system<Cs...>::hierarchy_modifier::hierarchy_modifier(
    hierarchy_system* system,
    entity ent
)
    : system_{system}, entity_{ent} {}

template <typename... Cs>
void hierarchy_system<Cs...>::cleanup(
    entity ent
) {
    if (!registry_->template has<hierarchy_component>(ent)) {
        return;
    }

    auto& hierarchy_comp = registry_->template get<hierarchy_component>(ent);
    auto parent = invalid_entity;

    if (hierarchy_comp.has_parent()) {
        parent = hierarchy_comp.get_parent();
        if (registry_->template has<hierarchy_component>(parent)) {
            auto& parent_comp = registry_->template get<hierarchy_component>(parent);
            parent_comp.children_.erase(ent);
        }
    }

    const auto& children = hierarchy_comp.get_children();
    for (entity child : children) {
        if (registry_->template has<hierarchy_component>(child)) {
            auto& child_comp = registry_->template get<hierarchy_component>(child);
            child_comp.parent_ = invalid_entity;

            transform_system_->mark_world_dirty(child);
        }
    }
}

template <typename... Cs>
auto hierarchy_system<Cs...>::modify(entity ent) -> hierarchy_modifier {
    return hierarchy_modifier{this, ent};
}

template <typename... Cs>
auto hierarchy_system<Cs...>::get_hierarchy_depth(
    entity ent
) const -> size_t {
    constexpr int MAX_HIERARCHY_DEPTH = 64;

    int depth      = 0;
    entity current = ent;

    while (registry_->template has<hierarchy_component>(current)) {
        const auto& hierarchy_comp = registry_->template get<hierarchy_component>(current);
        if (!hierarchy_comp.has_parent()) {
            break;
        }
        current = hierarchy_comp.get_parent();
        depth++;

        if (depth >= MAX_HIERARCHY_DEPTH) {
            throw std::runtime_error("hierarchy depth is too deep");
        }
    }

    return depth;
}

template <typename... Cs>
auto hierarchy_system<Cs...>::hierarchy_modifier::set_parent(entity parent)
    -> hierarchy_modifier& {
    if (!parent.is_valid()) {
        throw std::invalid_argument("parent is not valid");
    }

    if (!entity_.is_valid()) {
        throw std::invalid_argument("child is not valid");
    }

    if (parent == entity_) {
        throw std::invalid_argument("parent and child cannot be the same");
    }

    if (system_->check_hierarchy_cycle(parent, entity_)) {
        throw std::invalid_argument("setting parent to child would create a hierarchy cycle");
    }

    if (system_->registry_->template has<hierarchy_component>(parent)) {
        auto& parent_component = system_->registry_->template get<hierarchy_component>(parent);
        parent_component.children_.insert(entity_);
    }

    if (system_->registry_->template has<hierarchy_component>(entity_)) {
        auto& child_component   = system_->registry_->template get<hierarchy_component>(entity_);
        child_component.parent_ = parent;

        system_->transform_system_->mark_world_dirty(entity_);
    }

    return *this;
}

template <typename... Cs>
auto hierarchy_system<Cs...>::hierarchy_modifier::remove_parent() -> hierarchy_modifier& {
    if (system_->registry_->template has<hierarchy_component>(entity_)) {
        auto& child_component = system_->registry_->template get<hierarchy_component>(entity_);

        if (system_->registry_->template has<hierarchy_component>(child_component.parent_)) {
            auto& parent_component =
                system_->registry_->template get<hierarchy_component>(child_component.parent_);
            parent_component.children_.erase(entity_);
        }

        child_component.parent_ = invalid_entity;

        system_->transform_system_->mark_world_dirty(entity_);
    }

    return *this;
}

template <typename... Cs>
auto hierarchy_system<Cs...>::check_hierarchy_cycle(entity parent, entity child) const -> bool {
    entity current = parent;

    while (current.is_valid() && registry_->template has<hierarchy_component>(current)) {
        const auto& current_component = registry_->template get<hierarchy_component>(current);
        if (current_component.get_parent() == child) {
            return true;
        }
        current = current_component.get_parent();
    }

    return false;
}

}  // namespace vw::gfx

#endif  // VW_GFX_HIERARCHY_SYSTEM_INL_H
