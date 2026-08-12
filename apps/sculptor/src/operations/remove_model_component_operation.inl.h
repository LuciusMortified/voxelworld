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
    auto& model_reg = world.resource<asset::model_registry>();

    if (!state_->scene.name_to_entity.contains(params_.name)) {
        return;
    }
    const auto ent         = state_->scene.name_to_entity[params_.name];
    const auto& model_comp = world.get<ecs::model_component>(ent);

    if (model_comp.has_model()) {
        saved_model_ = model_comp.get_model();
    }

    model_reg.erase(params_.name);
    world.modify(ent).without<ecs::model_component>();
    state_->file.has_unsaved_changes = true;
}

inline void remove_model_component_operation::undo() {
    auto& world        = engine_->get_world();
    auto& model_sys = world.system<ecs::model_system>();

    if (!state_->scene.name_to_entity.contains(params_.name)) {
        return;
    }
    const auto ent = state_->scene.name_to_entity[params_.name];

    world.modify(ent).with<ecs::model_component>();
    model_sys.modify(ent).set_model(saved_model_);
    state_->file.has_unsaved_changes = true;
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_REMOVE_MODEL_COMPONENT_OPERATION_INL_H
