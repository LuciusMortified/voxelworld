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
    world_type& world, registry_type& registry
)
    : world_(&world), registry_(&registry) {}

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

    system_->dirty_entities_.insert(entity_);
    system_->mark_children_world_dirty(entity_);

    auto& spatial_system = system_->world_->get_spatial_system();
    if (system_->registry_->template has<spatial_component>(entity_)) {
        spatial_system.mark_dirty(entity_);
    }

    return *this;
}

template <typename... Cs>
auto transform_system<Cs...>::transform_modifier::set_rotation(
    const vec3f& rotation
) -> transform_modifier& {
    if (!system_->registry_->template has<transform_component>(entity_)) {
        return *this;
    }

    auto& transform_comp = system_->registry_->template get<transform_component>(entity_);
    transform_comp.transform_.set_rotation(rotation);
    transform_comp.local_dirty_ = true;
    transform_comp.world_dirty_ = true;

    system_->dirty_entities_.insert(entity_);
    system_->mark_children_world_dirty(entity_);

    auto& spatial_system = system_->world_->get_spatial_system();
    if (system_->registry_->template has<spatial_component>(entity_)) {
        spatial_system.mark_dirty(entity_);
    }

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

    system_->dirty_entities_.insert(entity_);
    system_->mark_children_world_dirty(entity_);

    auto& spatial_system = system_->world_->get_spatial_system();
    if (system_->registry_->template has<spatial_component>(entity_)) {
        spatial_system.mark_dirty(entity_);
    }

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

    system_->dirty_entities_.insert(entity_);
    system_->mark_children_world_dirty(entity_);

    auto& spatial_system = system_->world_->get_spatial_system();
    if (system_->registry_->template has<spatial_component>(entity_)) {
        spatial_system.mark_dirty(entity_);
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

    system_->dirty_entities_.insert(entity_);
    system_->mark_children_world_dirty(entity_);

    auto& spatial_system = system_->world_->get_spatial_system();
    if (spatial_system && system_->registry_->template has<spatial_component>(entity_)) {
        spatial_system->mark_dirty(entity_);
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

    system_->dirty_entities_.insert(entity_);
    system_->mark_children_world_dirty(entity_);

    auto& spatial_system = system_->world_->get_spatial_system();
    if (spatial_system && system_->registry_->template has<spatial_component>(entity_)) {
        spatial_system->mark_dirty(entity_);
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

    system_->dirty_entities_.insert(entity_);
    system_->mark_children_world_dirty(entity_);

    auto& spatial_system = system_->world_->get_spatial_system();
    if (spatial_system && system_->registry_->template has<spatial_component>(entity_)) {
        spatial_system->mark_dirty(entity_);
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

    dirty_entities_.insert(ent);
    mark_children_world_dirty(ent);

    if (registry_->template has<spatial_component>(ent)) {
        world_->get_spatial_system().mark_dirty(ent);
    }
}

template <typename... Cs>
void transform_system<Cs...>::mark_dirty(
    entity ent
) {
    dirty_entities_.insert(ent);
}

template <typename... Cs>
void transform_system<Cs...>::update() {
    if (dirty_entities_.empty()) {
        return;
    }

    sorted_dirty_entities_.assign(dirty_entities_.begin(), dirty_entities_.end());

    auto& hierarchy_sys = world_->get_hierarchy_system();
    std::ranges::sort(sorted_dirty_entities_, [this, &hierarchy_sys](entity lhs, entity rhs) {
        return hierarchy_sys.get_hierarchy_depth(lhs) < hierarchy_sys.get_hierarchy_depth(rhs);
    });

    for (entity ent : sorted_dirty_entities_) {
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
    sorted_dirty_entities_.clear();
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
            dirty_entities_.insert(child);
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

}  // namespace vw::gfx

#endif  // VW_GFX_TRANSFORM_SYSTEM_INL_H