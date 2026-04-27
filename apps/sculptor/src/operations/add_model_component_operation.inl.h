#pragma once

#ifndef VW_SCULPTOR_ADD_MODEL_COMPONENT_OPERATION_INL_H
#define VW_SCULPTOR_ADD_MODEL_COMPONENT_OPERATION_INL_H

namespace vw::sculptor {

inline add_model_component_operation::add_model_component_operation(
    engine_type& engine, app_state& state, const add_model_component_params& params
)
    : engine_(&engine), state_(&state), params_(params) {}

inline void add_model_component_operation::execute() {
    auto& world          = engine_->get_world();
    auto& model_reg = world.template get_resource<asset::model_registry>();
    auto& model_sys = world.template get_system<gfx::model_system>();

    if (!state_->scene.name_to_entity.contains(params_.name)) {
        return;
    }
    const auto ent = state_->scene.name_to_entity[params_.name];

    world.template add_component<gfx::model_component>(ent);

    const auto model = model_reg.create(params_.name, params_.size);
    model->fill(voxel{state_->tool.selected_block});

    model_sys.modify(ent).set_model(model);
    state_->file.has_unsaved_changes = true;
}

inline void add_model_component_operation::undo() {
    auto& world          = engine_->get_world();
    auto& model_reg = world.template get_resource<asset::model_registry>();

    if (!state_->scene.name_to_entity.contains(params_.name)) {
        return;
    }
    const auto ent = state_->scene.name_to_entity[params_.name];

    model_reg.erase(params_.name);
    world.template remove_component<gfx::model_component>(ent);
    state_->file.has_unsaved_changes = true;
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_ADD_MODEL_COMPONENT_OPERATION_INL_H
