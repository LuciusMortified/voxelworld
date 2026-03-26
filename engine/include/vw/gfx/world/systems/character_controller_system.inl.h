#pragma once

#ifndef VW_GFX_WORLD_SYSTEMS_CHARACTER_CONTROLLER_SYSTEM_INL_H
#define VW_GFX_WORLD_SYSTEMS_CHARACTER_CONTROLLER_SYSTEM_INL_H

namespace vw::gfx {

template <typename... Cs>
character_controller_system<Cs...>::character_controller_system(
    registry_type& registry
)
    : registry_(&registry) {}

template <typename... Cs>
void character_controller_system<Cs...>::update() {
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
