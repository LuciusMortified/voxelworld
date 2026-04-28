#pragma once

#ifndef VW_SCULPTOR_REMOVE_ANIMATION_TARGET_OPERATION_INL_H
#define VW_SCULPTOR_REMOVE_ANIMATION_TARGET_OPERATION_INL_H

namespace vw::sculptor {

inline remove_animation_target_operation::remove_animation_target_operation(
    engine_type& engine, app_state& state, const remove_animation_target_params& params
)
    : engine_(&engine), state_(&state), params_(params) {}

inline auto remove_animation_target_operation::find_animation_root_(
    ecs::entity ent
) const -> ecs::entity {
    auto& world         = engine_->get_world();
    ecs::entity current = ent;
    while (current.is_valid()) {
        if (world.has<gfx::animation_player_component>(current)) {
            return current;
        }
        if (!world.has<gfx::hierarchy_component>(current)) {
            break;
        }
        const auto& hier = world.get<gfx::hierarchy_component>(current);
        if (!hier.has_parent()) {
            break;
        }
        current = hier.get_parent();
    }
    return gfx::invalid_entity;
}

inline void remove_animation_target_operation::execute() {
    auto& world    = engine_->get_world();
    auto& anim_sys = world.template system<gfx::animation_system>();

    const auto ent          = state_->scene.name_to_entity[params_.entity_name];
    const auto& target_comp = world.get<gfx::animation_target_component>(ent);
    saved_target_name_      = target_comp.get_name();

    const auto root = find_animation_root_(ent);

    world.modify(ent).template without<gfx::animation_target_component>();

    if (root.is_valid()) {
        anim_sys.modify_player(root).rebuild_target_map();
    }
    state_->file.has_unsaved_changes = true;
}

inline void remove_animation_target_operation::undo() {
    auto& world    = engine_->get_world();
    auto& anim_sys = world.template system<gfx::animation_system>();

    if (!state_->scene.name_to_entity.contains(params_.entity_name)) {
        return;
    }
    const auto ent = state_->scene.name_to_entity[params_.entity_name];

    world.modify(ent).template with<gfx::animation_target_component>();
    auto target_mod = anim_sys.modify_target(ent);
    target_mod.set_target_name(saved_target_name_);
    if (world.has<gfx::transform_component>(ent)) {
        target_mod.set_rest_transform(
            world.get<gfx::transform_component>(ent).get_transform()
        );
    }

    const auto root = find_animation_root_(ent);
    if (root.is_valid()) {
        anim_sys.modify_player(root).rebuild_target_map();
    }
    state_->file.has_unsaved_changes = true;
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_REMOVE_ANIMATION_TARGET_OPERATION_INL_H
