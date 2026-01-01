#pragma once

#ifndef VW_GFX_TRANSFORM_SYSTEM_INL_H
#define VW_GFX_TRANSFORM_SYSTEM_INL_H

#include <algorithm>
#include <cmath>
#include <ranges>
#include <stdexcept>

#include "vw/gfx/world/components/hierarchy_component.h"
#include "vw/gfx/world/components/spatial_component.h"
#include "vw/gfx/world/components/transform_component.h"
#include "vw/gfx/world/systems/transform_system.h"

namespace vw::gfx {

template <typename... Cs>
inline transform_system<Cs...>::transform_system(
    registry_type& registry,
    spatial_system_type& spatial_sys
)
    : registry_(&registry), spatial_system_(&spatial_sys) {}

template <typename... Cs>
inline transform_system<Cs...>::transform_modifier::transform_modifier(
    transform_system* system, entity ent
)
    : system_(system), entity_(ent) {}

template <typename... Cs>
inline auto transform_system<Cs...>::modify(
    entity ent
) -> transform_modifier {
    return transform_modifier(this, ent);
}
template <typename... Cs>
auto transform_system<Cs...>::get_render_dirty_entities() -> std::unordered_set<entity>& {
    return render_dirty_entities_;
}

template <typename... Cs>
void transform_system<Cs...>::mark_render_dirty(
    entity ent
) {
    render_dirty_entities_.insert(ent);
}

template <typename... Cs>
inline auto transform_system<Cs...>::transform_modifier::set_position(
    const vec3f& position
) -> transform_modifier& {
    if (!system_->registry_->template has<transform_component>(entity_)) {
        return *this;
    }

    auto& transform_comp = system_->registry_->template get<transform_component>(entity_);
    transform_comp.transform_.set_position(position);
    transform_comp.local_dirty_ = true;
    transform_comp.world_dirty_ = true;

    system_->dirty_entities_.push_back(entity_);
    system_->mark_children_world_dirty(entity_);
    
    // Пометить spatial_component как dirty
    if (system_->spatial_system_ &&
        system_->registry_->template has<spatial_component>(entity_)) {
        system_->spatial_system_->mark_dirty(entity_);
    }

    return *this;
}

template <typename... Cs>
inline auto transform_system<Cs...>::transform_modifier::set_rotation(
    const vec3f& rotation
) -> transform_modifier& {
    if (!system_->registry_->template has<transform_component>(entity_)) {
        return *this;
    }

    auto& transform_comp = system_->registry_->template get<transform_component>(entity_);
    transform_comp.transform_.set_rotation(rotation);
    transform_comp.local_dirty_ = true;
    transform_comp.world_dirty_ = true;

    system_->dirty_entities_.push_back(entity_);
    system_->mark_children_world_dirty(entity_);
    
    // Пометить spatial_component как dirty
    if (system_->spatial_system_ &&
        system_->registry_->template has<spatial_component>(entity_)) {
        system_->spatial_system_->mark_dirty(entity_);
    }

    return *this;
}

template <typename... Cs>
inline auto transform_system<Cs...>::transform_modifier::set_scale(
    const vec3f& scale
) -> transform_modifier& {
    if (!system_->registry_->template has<transform_component>(entity_)) {
        return *this;
    }

    auto& transform_comp = system_->registry_->template get<transform_component>(entity_);
    transform_comp.transform_.set_scale(scale);
    transform_comp.local_dirty_ = true;
    transform_comp.world_dirty_ = true;

    system_->dirty_entities_.push_back(entity_);
    system_->mark_children_world_dirty(entity_);
    
    // Пометить spatial_component как dirty
    if (system_->spatial_system_ &&
        system_->registry_->template has<spatial_component>(entity_)) {
        system_->spatial_system_->mark_dirty(entity_);
    }

    return *this;
}

template <typename... Cs>
inline auto transform_system<Cs...>::transform_modifier::set_origin(
    const vec3f& origin
) -> transform_modifier& {
    if (!system_->registry_->template has<transform_component>(entity_)) {
        return *this;
    }

    auto& transform_comp = system_->registry_->template get<transform_component>(entity_);
    transform_comp.transform_.set_origin(origin);
    transform_comp.local_dirty_ = true;
    transform_comp.world_dirty_ = true;

    system_->dirty_entities_.push_back(entity_);
    system_->mark_children_world_dirty(entity_);
    
    // Пометить spatial_component как dirty
    if (system_->spatial_system_ &&
        system_->registry_->template has<spatial_component>(entity_)) {
        system_->spatial_system_->mark_dirty(entity_);
    }

    return *this;
}

template <typename... Cs>
inline auto transform_system<Cs...>::transform_modifier::translate(
    const vec3f& offset
) -> transform_modifier& {
    if (!system_->registry_->template has<transform_component>(entity_)) {
        return *this;
    }

    auto& transform_comp = system_->registry_->template get<transform_component>(entity_);
    transform_comp.transform_.translate(offset);
    transform_comp.local_dirty_ = true;
    transform_comp.world_dirty_ = true;

    system_->dirty_entities_.push_back(entity_);
    system_->mark_children_world_dirty(entity_);
    
    // Пометить spatial_component как dirty
    if (system_->spatial_system_ &&
        system_->registry_->template has<spatial_component>(entity_)) {
        system_->spatial_system_->mark_dirty(entity_);
    }

    return *this;
}

template <typename... Cs>
inline auto transform_system<Cs...>::transform_modifier::rotate(
    const vec3f& angles
) -> transform_modifier& {
    if (!system_->registry_->template has<transform_component>(entity_)) {
        return *this;
    }

    auto& transform_comp = system_->registry_->template get<transform_component>(entity_);
    transform_comp.transform_.rotate(angles);
    transform_comp.local_dirty_ = true;
    transform_comp.world_dirty_ = true;

    system_->dirty_entities_.push_back(entity_);
    system_->mark_children_world_dirty(entity_);
    
    // Пометить spatial_component как dirty
    if (system_->spatial_system_ &&
        system_->registry_->template has<spatial_component>(entity_)) {
        system_->spatial_system_->mark_dirty(entity_);
    }

    return *this;
}

template <typename... Cs>
inline auto transform_system<Cs...>::transform_modifier::scale(
    const vec3f& factor
) -> transform_modifier& {
    if (!system_->registry_->template has<transform_component>(entity_)) {
        return *this;
    }

    auto& transform_comp = system_->registry_->template get<transform_component>(entity_);
    transform_comp.transform_.scale(factor);
    transform_comp.local_dirty_ = true;
    transform_comp.world_dirty_ = true;

    system_->dirty_entities_.push_back(entity_);
    system_->mark_children_world_dirty(entity_);
    
    // Пометить spatial_component как dirty
    if (system_->spatial_system_ &&
        system_->registry_->template has<spatial_component>(entity_)) {
        system_->spatial_system_->mark_dirty(entity_);
    }

    return *this;
}

template <typename... Cs>
inline void transform_system<Cs...>::mark_world_dirty(
    entity ent
) {
    if (!registry_->template has<transform_component>(ent)) {
        return;
    }

    auto& transform_comp        = registry_->template get<transform_component>(ent);
    transform_comp.world_dirty_ = true;

    dirty_entities_.push_back(ent);
    mark_children_world_dirty(ent);
    
    // Пометить spatial_component как dirty
    if (spatial_system_ && registry_->template has<spatial_component>(ent)) {
        spatial_system_->mark_dirty(ent);
    }
}

template <typename... Cs>
void transform_system<Cs...>::mark_dirty(
    entity ent
) {
    dirty_entities_.push_back(ent);
}

template <typename... Cs>
inline void transform_system<Cs...>::update() {
    if (dirty_entities_.empty()) {
        return;
    }

    std::ranges::sort(dirty_entities_, [this](entity lhs, entity rhs) {
        return get_hierarchy_depth(lhs) < get_hierarchy_depth(rhs);
    });

    for (entity ent : dirty_entities_) {
        if (!registry_->template has<transform_component>(ent)) {
            continue;
        }

        auto& transform_comp = registry_->template get<transform_component>(ent);

        if (transform_comp.local_dirty_) {
            transform_comp.local_matrix_ = transform_comp.transform_.calc_matrix();
            transform_comp.local_dirty_  = false;
        }

        if (transform_comp.world_dirty_) {
            update_entity_world_matrix(ent, transform_comp);
            transform_comp.world_dirty_ = false;
        }

        render_dirty_entities_.insert(ent);
    }

    dirty_entities_.clear();
}

template <typename... Cs>
inline void transform_system<Cs...>::mark_children_world_dirty(
    entity ent
) {
    if (!registry_->template has<hierarchy_component>(ent)) {
        return;
    }

    const auto& hierarchy_comp = registry_->template get<hierarchy_component>(ent);
    const auto& children       = hierarchy_comp.get_children();

    for (entity child : children) {
        if (registry_->template has<transform_component>(child)) {
            auto& child_transform        = registry_->template get<transform_component>(child);
            child_transform.world_dirty_ = true;
            dirty_entities_.push_back(child);
        }
        mark_children_world_dirty(child);
    }
}
template <typename... Cs>
void transform_system<Cs...>::update_entity_world_matrix(
    entity ent, const transform_component& transform_comp
) {
    mat4f local_matrix           = transform_comp.get_local_matrix();
    transform_comp.world_matrix_ = local_matrix;

    if (registry_->template has<hierarchy_component>(ent)) {
        const auto& hierarchy_comp = registry_->template get<hierarchy_component>(ent);
        if (hierarchy_comp.has_parent()) {
            entity parent = hierarchy_comp.get_parent();
            if (registry_->template has<transform_component>(parent)) {
                const auto& parent_transform_comp =
                    registry_->template get<transform_component>(parent);
                transform_comp.world_matrix_ =
                    parent_transform_comp.get_world_matrix() * local_matrix;
            }
        }
    }
}

template <typename... Cs>
inline auto transform_system<Cs...>::get_hierarchy_depth(
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

}  // namespace vw::gfx

#endif  // VW_GFX_TRANSFORM_SYSTEM_INL_H