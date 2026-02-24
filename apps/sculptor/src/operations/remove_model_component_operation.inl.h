#pragma once

#ifndef VW_SCULPTOR_REMOVE_MODEL_COMPONENT_OPERATION_INL_H
#define VW_SCULPTOR_REMOVE_MODEL_COMPONENT_OPERATION_INL_H

namespace vw::sculptor {

inline remove_model_component_operation::remove_model_component_operation(
    engine_type& engine, app_state& state, const remove_model_component_params& params
)
    : engine_(&engine), state_(&state), params_(params) {}

inline void remove_model_component_operation::execute() {
    auto& world          = engine_->get_world();
    auto& model_registry = world.get_model_registry();

    const auto ent      = state_->scene.name_to_entity[params_.name];
    const auto& model_comp = world.get_component<gfx::model_component>(ent);

    if (model_comp.has_model()) {
        saved_size_   = model_comp.size();
        saved_voxels_ = model_comp.get_voxels();
    }

    auto* guard = state_->scene.find_guard(ent);
    if (!guard) {
        return;
    }

    model_registry.erase(params_.name);
    guard->without<gfx::model_component>();
    state_->file.has_unsaved_changes = true;
}

inline void remove_model_component_operation::undo() {
    auto& world          = engine_->get_world();
    auto& model_registry = world.get_model_registry();
    auto& model_system   = world.get_model_system();

    const auto ent = state_->scene.name_to_entity[params_.name];
    auto* guard    = state_->scene.find_guard(ent);
    if (!guard) {
        return;
    }

    guard->with<gfx::model_component>();

    const auto model = model_registry.create(params_.name, saved_size_);
    model->set_voxels(saved_voxels_);
    model_system.modify(ent).set_model(model);
    state_->file.has_unsaved_changes = true;
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_REMOVE_MODEL_COMPONENT_OPERATION_INL_H
