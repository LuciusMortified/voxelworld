#pragma once

#ifndef VW_SCULPTOR_REMOVE_VOXEL_OPERATION_INL_H
#define VW_SCULPTOR_REMOVE_VOXEL_OPERATION_INL_H

#include "vw/core/voxel.h"

namespace vw::sculptor {

inline remove_voxel_operation::remove_voxel_operation(
    engine_type& eng, app_state& st, const remove_voxel_params& params
)
    : engine_(&eng), state_(&st), params_(params) {}

inline void remove_voxel_operation::execute() {
    auto ent = state_->name_to_entity[params_.name];

    auto& world        = engine_->get_world();
    auto& model_system = world.get_model_system();

    auto& model_comp = world.template get_component<gfx::model_component>(ent);
    previous_color_  = model_comp.get_voxel(params_.position).value;

    model_system.modify(ent).set_voxel(params_.position, empty_voxel);
}

inline void remove_voxel_operation::undo() {
    auto ent = state_->name_to_entity[params_.name];

    auto& world        = engine_->get_world();
    auto& model_system = world.get_model_system();

    model_system.modify(ent).set_voxel(params_.position, previous_color_);
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_REMOVE_VOXEL_OPERATION_INL_H
