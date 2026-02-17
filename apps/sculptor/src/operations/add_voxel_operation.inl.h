#pragma once

#ifndef VW_SCULPTOR_ADD_VOXEL_OPERATION_INL_H
#define VW_SCULPTOR_ADD_VOXEL_OPERATION_INL_H

namespace vw::sculptor {

inline add_voxel_operation::add_voxel_operation(
    engine_type& eng, app_state& st, const add_voxel_params& params
)
    : engine_(&eng), state_(&st), params_(params) {}

inline void add_voxel_operation::execute() {
    auto ent = state_->name_to_entity[params_.name];

    auto& world        = engine_->get_world();
    auto& model_system = world.get_model_system();

    model_system.modify(ent).set_voxel(params_.position, params_.new_color);
    state_->has_unsaved_changes = true;
}

inline void add_voxel_operation::undo() {
    auto ent = state_->name_to_entity[params_.name];

    auto& world        = engine_->get_world();
    auto& model_system = world.get_model_system();

    model_system.modify(ent).set_voxel(params_.position, empty_voxel);
    state_->has_unsaved_changes = true;
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_ADD_VOXEL_OPERATION_INL_H
