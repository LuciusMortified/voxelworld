#pragma once

#ifndef VW_SCULPTOR_REMOVE_ANIMATION_TARGET_OPERATION_INL_H
#define VW_SCULPTOR_REMOVE_ANIMATION_TARGET_OPERATION_INL_H

namespace vw::sculptor {

inline remove_animation_target_operation::remove_animation_target_operation(
    engine_type& engine, app_state& state, const remove_animation_target_params& params
)
    : engine_(&engine), state_(&state), params_(params) {}

inline auto remove_animation_target_operation::find_animation_root_(
    gfx::entity ent
) const -> gfx::entity {
    auto& world         = engine_->get_world();
    gfx::entity current = ent;
    while (current.is_valid()) {
        if (world.has_component<gfx::animation_player_component>(current)) {
            return current;
        }
        if (!world.has_component<gfx::hierarchy_component>(current)) {
            break;
        }
        const auto& hier = world.get_component<gfx::hierarchy_component>(current);
        if (!hier.has_parent()) {
            break;
        }
        current = hier.get_parent();
    }
    return gfx::invalid_entity;
}

inline void remove_animation_target_operation::execute() {
    auto& world    = engine_->get_world();
    auto& anim_sys = world.get_animation_system();

    const auto ent          = state_->scene.name_to_entity[params_.entity_name];
    const auto& target_comp = world.get_component<gfx::animation_target_component>(ent);
    saved_target_name_      = target_comp.get_name();

    const auto root = find_animation_root_(ent);

    auto* guard = state_->scene.find_guard(ent);
    if (!guard) {
        return;
    }

    guard->without<gfx::animation_target_component>();

    if (root.is_valid()) {
        anim_sys.modify_player(root).rebuild_target_map();
    }
    state_->file.has_unsaved_changes = true;
}

inline void remove_animation_target_operation::undo() {
    auto& world    = engine_->get_world();
    auto& anim_sys = world.get_animation_system();

    const auto ent = state_->scene.name_to_entity[params_.entity_name];
    auto* guard    = state_->scene.find_guard(ent);
    if (!guard) {
        return;
    }

    guard->with<gfx::animation_target_component>();
    anim_sys.modify_target(ent).set_target_name(saved_target_name_);

    const auto root = find_animation_root_(ent);
    if (root.is_valid()) {
        anim_sys.modify_player(root).rebuild_target_map();
    }
    state_->file.has_unsaved_changes = true;
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_REMOVE_ANIMATION_TARGET_OPERATION_INL_H
