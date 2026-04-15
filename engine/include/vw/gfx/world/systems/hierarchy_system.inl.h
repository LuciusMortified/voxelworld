#pragma once

#include "vw/gfx/world/entity.h"
#ifndef VW_GFX_HIERARCHY_SYSTEM_INL_H
#define VW_GFX_HIERARCHY_SYSTEM_INL_H

#include <stdexcept>

#include "vw/gfx/world/systems/transform_system.h"

namespace vw::gfx {

template <typename WD>
hierarchy_system<WD>::hierarchy_system(context_type& context)
    : context_{&context} {}

template <typename WD>
void hierarchy_system<WD>::update(float32 /*dt*/) {}

template <typename WD>
hierarchy_system<WD>::hierarchy_modifier::hierarchy_modifier(
    hierarchy_system* system,
    entity ent
)
    : system_{system}, entity_{ent} {}

template <typename WD>
void hierarchy_system<WD>::cleanup(
    entity ent
) {
    if (!context_->registry().template has<hierarchy_component>(ent)) {
        return;
    }

    auto& hierarchy_comp = context_->registry().template get<hierarchy_component>(ent);
    auto parent = invalid_entity;

    if (hierarchy_comp.has_parent()) {
        parent = hierarchy_comp.get_parent();
        if (context_->registry().template has<hierarchy_component>(parent)) {
            auto& parent_comp = context_->registry().template get<hierarchy_component>(parent);
            std::erase(parent_comp.children_, ent);
        }
    }

    const auto& children = hierarchy_comp.get_children();
    for (entity child : children) {
        if (context_->registry().template has<hierarchy_component>(child)) {
            auto& child_comp = context_->registry().template get<hierarchy_component>(child);
            child_comp.parent_ = invalid_entity;

            context_->template get_system<transform_system>().modify(child).mark_world_dirty();
        }
    }
}

template <typename WD>
auto hierarchy_system<WD>::modify(entity ent) -> hierarchy_modifier {
    return hierarchy_modifier{this, ent};
}

template <typename WD>
auto hierarchy_system<WD>::get_hierarchy_depth(
    entity ent
) const -> size_t {
    constexpr int MAX_HIERARCHY_DEPTH = 64;

    int depth      = 0;
    entity current = ent;

    while (context_->registry().template has<hierarchy_component>(current)) {
        const auto& hierarchy_comp = context_->registry().template get<hierarchy_component>(current);
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

template <typename WD>
auto hierarchy_system<WD>::hierarchy_modifier::set_parent(entity parent)
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

    if (system_->context_->registry().template has<hierarchy_component>(parent)) {
        auto& parent_component = system_->context_->registry().template get<hierarchy_component>(parent);
        parent_component.children_.push_back(entity_);
    }

    if (system_->context_->registry().template has<hierarchy_component>(entity_)) {
        auto& child_component   = system_->context_->registry().template get<hierarchy_component>(entity_);
        child_component.parent_ = parent;

        system_->context_->template get_system<transform_system>().modify(entity_).mark_world_dirty();
    }

    return *this;
}

template <typename WD>
auto hierarchy_system<WD>::hierarchy_modifier::remove_parent() -> hierarchy_modifier& {
    if (system_->context_->registry().template has<hierarchy_component>(entity_)) {
        auto& child_component = system_->context_->registry().template get<hierarchy_component>(entity_);

        if (system_->context_->registry().template has<hierarchy_component>(child_component.parent_)) {
            auto& parent_component =
                system_->context_->registry().template get<hierarchy_component>(child_component.parent_);
            std::erase_if(parent_component.children_, [this](entity e) {
                return e == entity_;
            });
        }

        child_component.parent_ = invalid_entity;

        system_->context_->template get_system<transform_system>().modify(entity_).mark_world_dirty();
    }

    return *this;
}

template <typename WD>
auto hierarchy_system<WD>::check_hierarchy_cycle(entity parent, entity child) const -> bool {
    entity current = parent;

    while (current.is_valid() && context_->registry().template has<hierarchy_component>(current)) {
        const auto& current_component = context_->registry().template get<hierarchy_component>(current);
        if (current_component.get_parent() == child) {
            return true;
        }
        current = current_component.get_parent();
    }

    return false;
}

}  // namespace vw::gfx

#endif  // VW_GFX_HIERARCHY_SYSTEM_INL_H
