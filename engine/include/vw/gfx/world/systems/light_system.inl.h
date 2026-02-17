#pragma once

#ifndef VW_GFX_WORLD_SYSTEMS_LIGHT_SYSTEM_INL_H
#define VW_GFX_WORLD_SYSTEMS_LIGHT_SYSTEM_INL_H

#include "vw/gfx/world/systems/light_system.h"

namespace vw::gfx {

template <typename... Cs>
light_system<Cs...>::light_system(registry_type& registry)
    : registry_(&registry) {}

template <typename... Cs>
void light_system<Cs...>::update() {}

template <typename... Cs>
void light_system<Cs...>::mark_dirty(entity ent) {
    dirty_entities_.insert(ent);
}

template <typename... Cs>
light_system<Cs...>::light_modifier::light_modifier(
    light_system* system, entity ent
)
    : system_(system), entity_(ent) {}

template <typename... Cs>
auto light_system<Cs...>::modify(entity ent) -> light_modifier {
    return light_modifier(this, ent);
}

template <typename... Cs>
auto light_system<Cs...>::light_modifier::set_color(
    const vec3f& color
) -> light_modifier& {
    if (!system_->registry_->template has<light_component>(entity_)) {
        return *this;
    }
    auto& comp = system_->registry_->template get<light_component>(entity_);
    comp.color_ = color;
    system_->mark_render_dirty(entity_);
    return *this;
}

template <typename... Cs>
auto light_system<Cs...>::light_modifier::set_intensity(
    float32 intensity
) -> light_modifier& {
    if (!system_->registry_->template has<light_component>(entity_)) {
        return *this;
    }
    auto& comp = system_->registry_->template get<light_component>(entity_);
    comp.intensity_ = intensity;
    system_->mark_render_dirty(entity_);
    return *this;
}

template <typename... Cs>
auto light_system<Cs...>::light_modifier::set_range(
    float32 range
) -> light_modifier& {
    if (!system_->registry_->template has<light_component>(entity_)) {
        return *this;
    }
    auto& comp = system_->registry_->template get<light_component>(entity_);
    comp.range_ = range;
    system_->mark_render_dirty(entity_);
    return *this;
}

template <typename... Cs>
auto light_system<Cs...>::light_modifier::set_attenuation(
    float32 constant, float32 linear, float32 quadratic
) -> light_modifier& {
    if (!system_->registry_->template has<light_component>(entity_)) {
        return *this;
    }
    auto& comp = system_->registry_->template get<light_component>(entity_);
    comp.attenuation_constant_ = constant;
    comp.attenuation_linear_ = linear;
    comp.attenuation_quadratic_ = quadratic;
    system_->mark_render_dirty(entity_);
    return *this;
}

template <typename... Cs>
auto light_system<Cs...>::get_render_dirty_entities() -> std::unordered_set<entity>& {
    return render_dirty_entities_;
}

template <typename... Cs>
void light_system<Cs...>::mark_render_dirty(entity ent) {
    render_dirty_entities_.insert(ent);
}

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_SYSTEMS_LIGHT_SYSTEM_INL_H
