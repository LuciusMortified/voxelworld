#pragma once

#ifndef VW_GFX_TRANSFORM_SYSTEM_INL_H
#define VW_GFX_TRANSFORM_SYSTEM_INL_H

#include <algorithm>
#include <stdexcept>

#include "vw/gfx/world/components/hierarchy_component.h"
#include "vw/gfx/world/components/spatial_component.h"
#include "vw/gfx/world/components/transform_component.h"

namespace vw::gfx {

template <typename... Cs>
transform_system<Cs...>::transform_system(
    registry_type& registry,
    hierarchy_system<Cs...>& hierarchy_sys
)
    : registry_(&registry), hierarchy_system_(&hierarchy_sys) {}

template <typename... Cs>
transform_system<Cs...>::transform_modifier::transform_modifier(
    transform_system* system, entity ent
)
    : system_(system), entity_(ent) {}

template <typename... Cs>
auto transform_system<Cs...>::modify(
    entity ent
) -> transform_modifier {
    return transform_modifier(this, ent);
}

template <typename... Cs>
auto transform_system<Cs...>::transform_modifier::set_position(
    const vec3f& position
) -> transform_modifier& {
    if (!system_->registry_->template has<transform_component>(entity_)) {
        return *this;
    }

    auto& transform_comp = system_->registry_->template get<transform_component>(entity_);
    transform_comp.transform_.set_position(position);
    transform_comp.local_dirty_ = true;
    transform_comp.world_dirty_ = true;

    system_->registry_->template request_change<transform_component>(entity_);
    system_->mark_children_world_dirty(entity_);

    return *this;
}

template <typename... Cs>
auto transform_system<Cs...>::transform_modifier::set_rotation(
    const quat& rotation
) -> transform_modifier& {
    if (!system_->registry_->template has<transform_component>(entity_)) {
        return *this;
    }

    auto& transform_comp = system_->registry_->template get<transform_component>(entity_);
    transform_comp.transform_.set_rotation(rotation);
    transform_comp.local_dirty_ = true;
    transform_comp.world_dirty_ = true;

    system_->registry_->template request_change<transform_component>(entity_);
    system_->mark_children_world_dirty(entity_);

    return *this;
}

template <typename... Cs>
auto transform_system<Cs...>::transform_modifier::set_rotation_euler(
    const vec3f& euler
) -> transform_modifier& {
    if (!system_->registry_->template has<transform_component>(entity_)) {
        return *this;
    }

    auto& transform_comp = system_->registry_->template get<transform_component>(entity_);
    transform_comp.transform_.set_rotation_euler(euler);
    transform_comp.local_dirty_ = true;
    transform_comp.world_dirty_ = true;

    system_->registry_->template request_change<transform_component>(entity_);
    system_->mark_children_world_dirty(entity_);

    return *this;
}

template <typename... Cs>
auto transform_system<Cs...>::transform_modifier::set_scale(
    const vec3f& scale
) -> transform_modifier& {
    if (!system_->registry_->template has<transform_component>(entity_)) {
        return *this;
    }

    auto& transform_comp = system_->registry_->template get<transform_component>(entity_);
    transform_comp.transform_.set_scale(scale);
    transform_comp.local_dirty_ = true;
    transform_comp.world_dirty_ = true;

    system_->registry_->template request_change<transform_component>(entity_);
    system_->mark_children_world_dirty(entity_);

    return *this;
}

template <typename... Cs>
auto transform_system<Cs...>::transform_modifier::set_origin(
    const vec3f& origin
) -> transform_modifier& {
    if (!system_->registry_->template has<transform_component>(entity_)) {
        return *this;
    }

    auto& transform_comp = system_->registry_->template get<transform_component>(entity_);
    transform_comp.transform_.set_origin(origin);
    transform_comp.local_dirty_ = true;
    transform_comp.world_dirty_ = true;

    system_->registry_->template request_change<transform_component>(entity_);
    system_->mark_children_world_dirty(entity_);

    return *this;
}

template <typename... Cs>
auto transform_system<Cs...>::transform_modifier::translate(
    const vec3f& offset
) -> transform_modifier& {
    if (!system_->registry_->template has<transform_component>(entity_)) {
        return *this;
    }

    auto& transform_comp = system_->registry_->template get<transform_component>(entity_);
    transform_comp.transform_.translate(offset);
    transform_comp.local_dirty_ = true;
    transform_comp.world_dirty_ = true;

    system_->registry_->template request_change<transform_component>(entity_);
    system_->mark_children_world_dirty(entity_);

    return *this;
}

template <typename... Cs>
auto transform_system<Cs...>::transform_modifier::rotate(
    const vec3f& angles
) -> transform_modifier& {
    if (!system_->registry_->template has<transform_component>(entity_)) {
        return *this;
    }

    auto& transform_comp = system_->registry_->template get<transform_component>(entity_);
    transform_comp.transform_.rotate(angles);
    transform_comp.local_dirty_ = true;
    transform_comp.world_dirty_ = true;

    system_->registry_->template request_change<transform_component>(entity_);
    system_->mark_children_world_dirty(entity_);

    return *this;
}

template <typename... Cs>
auto transform_system<Cs...>::transform_modifier::scale(
    const vec3f& factor
) -> transform_modifier& {
    if (!system_->registry_->template has<transform_component>(entity_)) {
        return *this;
    }

    auto& transform_comp = system_->registry_->template get<transform_component>(entity_);
    transform_comp.transform_.scale(factor);
    transform_comp.local_dirty_ = true;
    transform_comp.world_dirty_ = true;

    system_->registry_->template request_change<transform_component>(entity_);
    system_->mark_children_world_dirty(entity_);

    return *this;
}

template <typename... Cs>
auto transform_system<Cs...>::transform_modifier::mark_world_dirty() -> transform_modifier& {
    if (!system_->registry_->template has<transform_component>(entity_)) {
        return *this;
    }

    auto& transform_comp        = system_->registry_->template get<transform_component>(entity_);
    transform_comp.world_dirty_ = true;

    system_->registry_->template request_change<transform_component>(entity_);
    system_->mark_children_world_dirty(entity_);

    return *this;
}

template <typename... Cs>
transform_system<Cs...>::transform_modifier& transform_system<Cs...>::transform_modifier::
    set_transform(
        const transform& transform
    ) {
    if (!system_->registry_->template has<transform_component>(entity_)) {
        return *this;
    }

    auto& transform_comp        = system_->registry_->template get<transform_component>(entity_);
    transform_comp.transform_   = transform;
    transform_comp.local_dirty_ = true;
    transform_comp.world_dirty_ = true;

    system_->registry_->template request_change<transform_component>(entity_);
    system_->mark_children_world_dirty(entity_);

    return *this;
}

template <typename... Cs>
transform_system<Cs...>::transform_modifier& transform_system<Cs...>::transform_modifier::
    set_transform_with_matrix(
        const transform& transform,
        const mat4f& local_matrix
    ) {
    if (!system_->registry_->template has<transform_component>(entity_)) {
        return *this;
    }

    auto& transform_comp          = system_->registry_->template get<transform_component>(entity_);
    transform_comp.transform_     = transform;
    transform_comp.local_matrix_  = local_matrix;
    transform_comp.local_dirty_   = false;
    transform_comp.world_dirty_   = true;

    system_->registry_->template request_change<transform_component>(entity_);
    system_->mark_children_world_dirty(entity_);

    return *this;
}

template <typename... Cs>
void transform_system<Cs...>::update() {
    auto& requested = registry_->template requested<transform_component>();
    if (requested.empty()) {
        return;
    }

    sorted_entities_.assign(requested.begin(), requested.end());

    std::ranges::sort(sorted_entities_, [this](entity lhs, entity rhs) {
        return hierarchy_system_->get_hierarchy_depth(lhs) <
               hierarchy_system_->get_hierarchy_depth(rhs);
    });

    for (entity ent : sorted_entities_) {
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

        registry_->template notify_changed<transform_component>(ent);
    }

    sorted_entities_.clear();
    registry_->template clear_requested<transform_component>();
}

template <typename... Cs>
void transform_system<Cs...>::mark_children_world_dirty(
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
            registry_->template request_change<transform_component>(child);
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

                auto parent_world_matrix = parent_transform_comp.get_world_matrix() *
                    math::translation_matrix(-parent_transform_comp.get_origin());

                transform_comp.world_matrix_ = parent_world_matrix * local_matrix;
            }
        }
    }
}

}  // namespace vw::gfx

#endif  // VW_GFX_TRANSFORM_SYSTEM_INL_H
