module vw.world;

import std;

namespace vw::ecs {

character_controller_system::character_controller_system(world& w)
    : world_(&w) {}

void character_controller_system::update(float32 delta_time) {
    auto& reg = world_->registry();
    for (auto [ent, cc, rb, mi] :
         reg.view<character_controller_component, rigid_body_component, movement_intent_component>()) {

        if (rb.is_frozen()) {
            cc.jump_requested_ = false;
            cc.move_input_ = {0.0f, 0.0f, 0.0f};
            continue;
        }

        mi.wish_velocity_.x = cc.move_input_.x * cc.move_speed_;
        mi.wish_velocity_.z = cc.move_input_.z * cc.move_speed_;
        mi.wish_axes_ = axis_flag::xz;

        if (cc.jump_requested_ && rb.is_grounded()) {
            mi.wish_velocity_.y = cc.jump_impulse_;
            mi.wish_axes_ = axis_flag::xz | axis_flag::y;
        }

        auto facing_len = math::length(cc.facing_direction_);
        if (facing_len > 0.001f && reg.has<transform_component>(ent)) {
            const auto& tc = reg.get<transform_component>(ent);
            auto target = math::quat_look_y(cc.facing_direction_);
            auto current = tc.get_rotation();
            float32 t = math::clamp(cc.rotation_speed_ * delta_time, 0.0f, 1.0f);
            world_->system<transform_system>().modify(ent).set_rotation(math::slerp(current, target, t));
        }

        cc.jump_requested_ = false;
        cc.move_input_ = {0.0f, 0.0f, 0.0f};
    }
}

character_controller_system::controller_modifier::controller_modifier(
    character_controller_system* system, entity ent
)
    : system_(system), entity_(ent) {}

auto character_controller_system::modify(
    entity ent
) -> controller_modifier {
    return controller_modifier(this, ent);
}

auto character_controller_system::controller_modifier::set_move_input(
    const vec3f& input
) -> controller_modifier& {
    auto& reg = system_->world_->registry();
    if (!reg.has<character_controller_component>(entity_)) {
        return *this;
    }
    auto& comp = reg.get<character_controller_component>(entity_);
    comp.move_input_ = input;
    return *this;
}

auto character_controller_system::controller_modifier::set_facing_direction(
    const vec3f& direction
) -> controller_modifier& {
    auto& reg = system_->world_->registry();
    if (!reg.has<character_controller_component>(entity_)) {
        return *this;
    }
    auto& comp = reg.get<character_controller_component>(entity_);
    comp.facing_direction_ = direction;
    return *this;
}

auto character_controller_system::controller_modifier::set_move_speed(
    float32 speed
) -> controller_modifier& {
    auto& reg = system_->world_->registry();
    if (!reg.has<character_controller_component>(entity_)) {
        return *this;
    }
    auto& comp = reg.get<character_controller_component>(entity_);
    comp.move_speed_ = speed;
    return *this;
}

auto character_controller_system::controller_modifier::set_jump_impulse(
    float32 impulse
) -> controller_modifier& {
    auto& reg = system_->world_->registry();
    if (!reg.has<character_controller_component>(entity_)) {
        return *this;
    }
    auto& comp = reg.get<character_controller_component>(entity_);
    comp.jump_impulse_ = impulse;
    return *this;
}

auto character_controller_system::controller_modifier::set_rotation_speed(
    float32 speed
) -> controller_modifier& {
    auto& reg = system_->world_->registry();
    if (!reg.has<character_controller_component>(entity_)) {
        return *this;
    }
    auto& comp = reg.get<character_controller_component>(entity_);
    comp.rotation_speed_ = speed;
    return *this;
}

auto character_controller_system::controller_modifier::request_jump(
) -> controller_modifier& {
    auto& reg = system_->world_->registry();
    if (!reg.has<character_controller_component>(entity_)) {
        return *this;
    }
    auto& comp = reg.get<character_controller_component>(entity_);
    comp.jump_requested_ = true;
    return *this;
}

}  // namespace vw::ecs
