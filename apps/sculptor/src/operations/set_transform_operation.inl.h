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
    auto& transform_system = world.get_transform_system();

    auto ent = state_->name_to_entity[params_.name];

    auto& transform_comp = world.template get_component<gfx::transform_component>(ent);
    previous_transform_  = transform_comp.get_transform();
    transform_system.modify(ent).set_transform(params_.transform);
    state_->has_unsaved_changes = true;
}

inline void set_transform_operation::undo() {
    auto& world            = engine_->get_world();
    auto& transform_system = world.get_transform_system();

    auto ent = state_->name_to_entity[params_.name];
    transform_system.modify(ent).set_transform(previous_transform_);
    state_->has_unsaved_changes = true;
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_SET_TRANSFORM_OPERATION_INL_H
