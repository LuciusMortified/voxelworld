#pragma once

#ifndef VW_ECS_SYSTEMS_LIGHT_SYSTEM_INL_H
#define VW_ECS_SYSTEMS_LIGHT_SYSTEM_INL_H

#include "vw/ecs/systems/light_system.h"
#include "vw/ecs/world.h"

namespace vw::ecs {

template <typename WD>
light_system<WD>::light_system(world_type& w)
    : world_(&w) {}

template <typename WD>
template <typename C>
    requires std::same_as<C, light_component>
void light_system<WD>::on_add(entity e) {
    world_->registry().template request_change<light_component>(e);
}

template <typename WD>
void light_system<WD>::update(float32 /*dt*/) {
    auto& reg       = world_->registry();
    auto& requested = reg.template requested<light_component>();
    if (requested.empty()) {
        return;
    }

    for (entity ent : requested) {
        reg.template notify_changed<light_component>(ent);
    }

    reg.template clear_requested<light_component>();
}

template <typename WD>
light_system<WD>::light_modifier::light_modifier(
    light_system* system, entity ent
)
    : system_(system), entity_(ent) {}

template <typename WD>
auto light_system<WD>::modify(entity ent) -> light_modifier {
    return light_modifier(this, ent);
}

template <typename WD>
auto light_system<WD>::light_modifier::set_color(
    const vec3f& color
) -> light_modifier& {
    auto& reg = system_->world_->registry();
    if (!reg.template has<light_component>(entity_)) {
        return *this;
    }
    auto& comp = reg.template get<light_component>(entity_);
    comp.color_ = color;
    reg.template request_change<light_component>(entity_);
    return *this;
}

template <typename WD>
auto light_system<WD>::light_modifier::set_intensity(
    float32 intensity
) -> light_modifier& {
    auto& reg = system_->world_->registry();
    if (!reg.template has<light_component>(entity_)) {
        return *this;
    }
    auto& comp = reg.template get<light_component>(entity_);
    comp.intensity_ = intensity;
    reg.template request_change<light_component>(entity_);
    return *this;
}

template <typename WD>
auto light_system<WD>::light_modifier::set_range(
    float32 range
) -> light_modifier& {
    auto& reg = system_->world_->registry();
    if (!reg.template has<light_component>(entity_)) {
        return *this;
    }
    auto& comp = reg.template get<light_component>(entity_);
    comp.range_ = range;
    reg.template request_change<light_component>(entity_);
    return *this;
}

template <typename WD>
auto light_system<WD>::light_modifier::set_attenuation(
    float32 constant, float32 linear, float32 quadratic
) -> light_modifier& {
    auto& reg = system_->world_->registry();
    if (!reg.template has<light_component>(entity_)) {
        return *this;
    }
    auto& comp = reg.template get<light_component>(entity_);
    comp.attenuation_constant_ = constant;
    comp.attenuation_linear_ = linear;
    comp.attenuation_quadratic_ = quadratic;
    reg.template request_change<light_component>(entity_);
    return *this;
}

}  // namespace vw::ecs

#endif  // VW_ECS_SYSTEMS_LIGHT_SYSTEM_INL_H
