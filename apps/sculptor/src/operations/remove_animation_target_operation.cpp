module vw.sculptor;

import std;

import vw.core;
import vw.ecs;
import vw.world;
import vw.platform;
import vw.gfx;

namespace vw::sculptor {

remove_animation_target_operation::remove_animation_target_operation(
    engine_type& engine, app_state& state, const remove_animation_target_params& params
)
    : engine_(&engine), state_(&state), params_(params) {}

auto remove_animation_target_operation::find_animation_root_(
    ecs::entity ent
) const -> ecs::entity {
    auto& world         = engine_->get_world();
    ecs::entity current = ent;
    while (current.is_valid()) {
        if (world.has<ecs::animation_player_component>(current)) {
            return current;
        }
        if (!world.has<ecs::hierarchy_component>(current)) {
            break;
        }
        const auto& hier = world.get<ecs::hierarchy_component>(current);
        if (!hier.has_parent()) {
            break;
        }
        current = hier.get_parent();
    }
    return ecs::invalid_entity;
}

auto remove_animation_target_operation::execute() -> void {
    auto& world    = engine_->get_world();
    auto& anim_sys = world.system<ecs::animation_system>();

    const auto ent          = state_->scene.name_to_entity[params_.entity_name];
    const auto& target_comp = world.get<ecs::animation_target_component>(ent);
    saved_target_name_      = target_comp.get_name();

    const auto root = find_animation_root_(ent);

    world.modify(ent).without<ecs::animation_target_component>();

    if (root.is_valid()) {
        anim_sys.modify_player(root).rebuild_target_map();
    }
    state_->file.has_unsaved_changes = true;
}

auto remove_animation_target_operation::undo() -> void {
    auto& world    = engine_->get_world();
    auto& anim_sys = world.system<ecs::animation_system>();

    if (!state_->scene.name_to_entity.contains(params_.entity_name)) {
        return;
    }
    const auto ent = state_->scene.name_to_entity[params_.entity_name];

    world.modify(ent).with<ecs::animation_target_component>();
    auto target_mod = anim_sys.modify_target(ent);
    target_mod.set_target_name(saved_target_name_);
    if (world.has<ecs::transform_component>(ent)) {
        target_mod.set_rest_transform(
            world.get<ecs::transform_component>(ent).get_transform()
        );
    }

    const auto root = find_animation_root_(ent);
    if (root.is_valid()) {
        anim_sys.modify_player(root).rebuild_target_map();
    }
    state_->file.has_unsaved_changes = true;
}

}  // namespace vw::sculptor
