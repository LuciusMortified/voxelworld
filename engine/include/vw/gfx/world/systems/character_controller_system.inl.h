#pragma once

#ifndef VW_GFX_WORLD_SYSTEMS_CHARACTER_CONTROLLER_SYSTEM_INL_H
#define VW_GFX_WORLD_SYSTEMS_CHARACTER_CONTROLLER_SYSTEM_INL_H

#include "vw/gfx/world/components/transform_component.h"
#include "vw/gfx/world/systems/transform_system.h"

namespace vw::gfx {

template <typename... Cs>
character_controller_system<Cs...>::character_controller_system(
    registry_type& registry, transform_system_type& transform_system
)
    : registry_(&registry)
    , transform_system_(&transform_system) {}

template <typename... Cs>
void character_controller_system<Cs...>::update(float32 delta_time) {
    for (auto [ent, cc_const, rb_const] :
         registry_->template view<character_controller_component, rigid_body_component>()) {
        auto& cc = registry_->template get<character_controller_component>(ent);
        auto& rb = registry_->template get<rigid_body_component>(ent);

        if (rb.frozen_) {
            cc.jump_requested_ = false;
            cc.move_input_ = {0.0f, 0.0f, 0.0f};
            continue;
        }

        rb.velocity_.x = cc.move_input_.x * cc.move_speed_;
        rb.velocity_.z = cc.move_input_.z * cc.move_speed_;

        if (cc.jump_requested_ && rb.grounded_) {
            rb.velocity_.y = cc.jump_impulse_;
            rb.grounded_ = false;
        }

        auto facing_len = math::length(cc.facing_direction_);
        if (facing_len > 0.001f && registry_->template has<transform_component>(ent)) {
            const auto& tc = registry_->template get<transform_component>(ent);
            auto target = math::quat_look_y(cc.facing_direction_);
            auto current = tc.get_rotation();
            float32 t = math::clamp(cc.rotation_speed_ * delta_time, 0.0f, 1.0f);
            transform_system_->modify(ent).set_rotation(math::slerp(current, target, t));
        }

        cc.jump_requested_ = false;
        cc.move_input_ = {0.0f, 0.0f, 0.0f};
    }
}

template <typename... Cs>
character_controller_system<Cs...>::controller_modifier::controller_modifier(
    character_controller_system* system, entity ent
)
    : system_(system), entity_(ent) {}

template <typename... Cs>
auto character_controller_system<Cs...>::modify(
    entity ent
) -> controller_modifier {
    return controller_modifier(this, ent);
}

template <typename... Cs>
auto character_controller_system<Cs...>::controller_modifier::set_move_input(
    const vec3f& input
) -> controller_modifier& {
    if (!system_->registry_->template has<character_controller_component>(entity_)) {
        return *this;
    }
    auto& comp = system_->registry_->template get<character_controller_component>(entity_);
    comp.move_input_ = input;
    return *this;
}

template <typename... Cs>
auto character_controller_system<Cs...>::controller_modifier::set_facing_direction(
    const vec3f& direction
) -> controller_modifier& {
    if (!system_->registry_->template has<character_controller_component>(entity_)) {
        return *this;
    }
    auto& comp = system_->registry_->template get<character_controller_component>(entity_);
    comp.facing_direction_ = direction;
    return *this;
}

template <typename... Cs>
auto character_controller_system<Cs...>::controller_modifier::set_move_speed(
    float32 speed
) -> controller_modifier& {
    if (!system_->registry_->template has<character_controller_component>(entity_)) {
        return *this;
    }
    auto& comp = system_->registry_->template get<character_controller_component>(entity_);
    comp.move_speed_ = speed;
    return *this;
}

template <typename... Cs>
auto character_controller_system<Cs...>::controller_modifier::set_jump_impulse(
    float32 impulse
) -> controller_modifier& {
    if (!system_->registry_->template has<character_controller_component>(entity_)) {
        return *this;
    }
    auto& comp = system_->registry_->template get<character_controller_component>(entity_);
    comp.jump_impulse_ = impulse;
    return *this;
}

template <typename... Cs>
auto character_controller_system<Cs...>::controller_modifier::set_rotation_speed(
    float32 speed
) -> controller_modifier& {
    if (!system_->registry_->template has<character_controller_component>(entity_)) {
        return *this;
    }
    auto& comp = system_->registry_->template get<character_controller_component>(entity_);
    comp.rotation_speed_ = speed;
    return *this;
}

template <typename... Cs>
auto character_controller_system<Cs...>::controller_modifier::request_jump(
) -> controller_modifier& {
    if (!system_->registry_->template has<character_controller_component>(entity_)) {
        return *this;
    }
    auto& comp = system_->registry_->template get<character_controller_component>(entity_);
    comp.jump_requested_ = true;
    return *this;
}

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_SYSTEMS_CHARACTER_CONTROLLER_SYSTEM_INL_H