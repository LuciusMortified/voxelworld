#pragma once

#ifndef VW_SCULPTOR_SET_TRANSFORM_OPERATION_INL_H
#define VW_SCULPTOR_SET_TRANSFORM_OPERATION_INL_H

namespace vw::sculptor {

inline set_transform_operation::set_transform_operation(
    engine_type& engine, app_state& st, const set_transform_params& params
)
    : engine_(&engine), state_(&st), params_(params) {}

inline void set_transform_operation::execute() {
    auto& world            = engine_->get_world();
    auto& transform_sys = world.system<ecs::transform_system>();

    auto ent = state_->scene.name_to_entity[params_.name];

    auto& transform_comp = world.get<ecs::transform_component>(ent);
    previous_transform_  = transform_comp.get_transform();
    transform_sys.modify(ent).set_transform(params_.new_transform);

    if (world.has<ecs::animation_target_component>(ent)) {
        world.system<ecs::animation_system>().modify_target(ent).set_rest_transform(params_.new_transform);
    }

    state_->file.has_unsaved_changes = true;
}

inline void set_transform_operation::undo() {
    auto& world            = engine_->get_world();
    auto& transform_sys = world.system<ecs::transform_system>();

    auto ent = state_->scene.name_to_entity[params_.name];
    transform_sys.modify(ent).set_transform(previous_transform_);

    if (world.has<ecs::animation_target_component>(ent)) {
        world.system<ecs::animation_system>().modify_target(ent).set_rest_transform(previous_transform_);
    }

    state_->file.has_unsaved_changes = true;
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_SET_TRANSFORM_OPERATION_INL_H
