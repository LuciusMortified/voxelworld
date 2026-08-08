#pragma once

#ifndef VW_SCULPTOR_CREATE_ENTITY_OPERATION_INL_H
#define VW_SCULPTOR_CREATE_ENTITY_OPERATION_INL_H

namespace vw::sculptor {

inline create_entity_operation::create_entity_operation(
    engine_type& engine, app_state& state, const create_entity_params& params
)
    : base_operation(), engine_(&engine), state_(&state), params_(params) {}

inline void create_entity_operation::execute() {
    auto& world            = engine_->get_world();
    auto& hierarchy_sys = world.system<gfx::hierarchy_system>();
    auto& transform_sys = world.system<gfx::transform_system>();
    auto& model_reg = world.resource<asset::model_registry>();
    auto& model_sys = world.system<gfx::model_system>();

    auto modifier = world.create()
        .with<gfx::hierarchy_component>()
        .with<gfx::transform_component>()
        .with<gfx::spatial_component>();

    std::shared_ptr<asset::model> model = nullptr;
    if (params_.with_model) {
        modifier.with<gfx::model_component>();

        model = model_reg.create(params_.name, params_.size);
        model->fill(voxel{state_->tool.selected_block});
    }

    if (params_.with_socket) {
        modifier.with<gfx::socket_component>();
    }

    const auto ent = modifier.get_entity();

    transform_sys.modify(ent)
        .set_origin(vec3f{-params_.size.x / 2.f, -params_.size.y / 2.f, -params_.size.z / 2.f});

    if (model) {
        model_sys.modify(ent).set_model(model);
    }

    if (!params_.parent_name.empty() && state_->scene.name_to_entity.contains(params_.parent_name)) {
        auto parent_ent = state_->scene.name_to_entity[params_.parent_name];
        hierarchy_sys.modify(ent).set_parent(parent_ent);
    }

    state_->scene.name_to_entity[params_.name] = ent;
    state_->scene.entity_to_name[ent]          = params_.name;

    if (state_->scene.root_name.empty()) {
        state_->scene.root_name = params_.name;
    }
    state_->scene.selected_name = params_.name;

    state_->scene.entities.push_back(ent);
    state_->file.has_unsaved_changes = true;
}

inline void create_entity_operation::undo() {
    auto& world = engine_->get_world();
    auto ent    = state_->scene.name_to_entity[params_.name];

    state_->scene.entity_to_name.erase(ent);
    state_->scene.name_to_entity.erase(params_.name);

    world.destroy(ent);
    std::erase(state_->scene.entities, ent);

    if (state_->scene.root_name == params_.name) {
        state_->scene.root_name = "";
    }
    state_->scene.selected_name       = "";
    state_->file.has_unsaved_changes = true;
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_CREATE_ENTITY_OPERATION_INL_H
