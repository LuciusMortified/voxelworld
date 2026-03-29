#pragma once

#ifndef VW_GFX_WORLD_SYSTEMS_LIGHT_SYSTEM_INL_H
#define VW_GFX_WORLD_SYSTEMS_LIGHT_SYSTEM_INL_H

#include "vw/gfx/world/systems/light_system.h"

namespace vw::gfx {

template <typename WC>
light_system<WC>::light_system(context_type& context)
    : context_(&context) {}

template <typename WC>
void light_system<WC>::update() {
    auto& requested = context_->registry.template requested<light_component>();
    if (requested.empty()) {
        return;
    }

    for (entity ent : requested) {
        context_->registry.template notify_changed<light_component>(ent);
    }

    context_->registry.template clear_requested<light_component>();
}

template <typename WC>
light_system<WC>::light_modifier::light_modifier(
    light_system* system, entity ent
)
    : system_(system), entity_(ent) {}

template <typename WC>
auto light_system<WC>::modify(entity ent) -> light_modifier {
    return light_modifier(this, ent);
}

template <typename WC>
auto light_system<WC>::light_modifier::set_color(
    const vec3f& color
) -> light_modifier& {
    if (!system_->context_->registry.template has<light_component>(entity_)) {
        return *this;
    }
    auto& comp = system_->context_->registry.template get<light_component>(entity_);
    comp.color_ = color;
    system_->context_->registry.template request_change<light_component>(entity_);
    return *this;
}

template <typename WC>
auto light_system<WC>::light_modifier::set_intensity(
    float32 intensity
) -> light_modifier& {
    if (!system_->context_->registry.template has<light_component>(entity_)) {
        return *this;
    }
    auto& comp = system_->context_->registry.template get<light_component>(entity_);
    comp.intensity_ = intensity;
    system_->context_->registry.template request_change<light_component>(entity_);
    return *this;
}

template <typename WC>
auto light_system<WC>::light_modifier::set_range(
    float32 range
) -> light_modifier& {
    if (!system_->context_->registry.template has<light_component>(entity_)) {
        return *this;
    }
    auto& comp = system_->context_->registry.template get<light_component>(entity_);
    comp.range_ = range;
    system_->context_->registry.template request_change<light_component>(entity_);
    return *this;
}

template <typename WC>
auto light_system<WC>::light_modifier::set_attenuation(
    float32 constant, float32 linear, float32 quadratic
) -> light_modifier& {
    if (!system_->context_->registry.template has<light_component>(entity_)) {
        return *this;
    }
    auto& comp = system_->context_->registry.template get<light_component>(entity_);
    comp.attenuation_constant_ = constant;
    comp.attenuation_linear_ = linear;
    comp.attenuation_quadratic_ = quadratic;
    system_->context_->registry.template request_change<light_component>(entity_);
    return *this;
}

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_SYSTEMS_LIGHT_SYSTEM_INL_H
