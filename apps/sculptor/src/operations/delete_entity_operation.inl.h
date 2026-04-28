#pragma once

#ifndef VW_SCULPTOR_DELETE_ENTITY_OPERATION_INL_H
#define VW_SCULPTOR_DELETE_ENTITY_OPERATION_INL_H

namespace vw::sculptor {

inline delete_entity_operation::delete_entity_operation(
    engine_type& engine, app_state& state, const delete_entity_params& params
)
    : engine_(&engine), state_(&state), params_(params) {}

inline void delete_entity_operation::execute() {
    auto ent = state_->scene.name_to_entity[params_.name];

    auto& world          = engine_->get_world();
    auto& hierarchy_comp = world.get<gfx::hierarchy_component>(ent);
    auto& transform_comp = world.get<gfx::transform_component>(ent);

    bool has_parent = hierarchy_comp.has_parent();
    if (has_parent) {
        parent_name_ = state_->scene.entity_to_name[hierarchy_comp.get_parent()];
    }

    transform_ = transform_comp.get_transform();

    if (world.has<gfx::model_component>(ent)) {
        auto& model_comp = world.get<gfx::model_component>(ent);

        if (model_comp.has_model()) {
            with_model_  = true;
            saved_model_ = model_comp.get_model();
        }
    }

    state_->scene.entity_to_name.erase(ent);
    state_->scene.name_to_entity.erase(params_.name);

    if (state_->scene.root_name == params_.name) {
        state_->scene.root_name = "";
    }
    if (has_parent) {
        state_->scene.selected_name = parent_name_;
    } else {
        state_->scene.selected_name = "";
    }

    world.destroy(ent);
    std::erase(state_->scene.entities, ent);
    state_->file.has_unsaved_changes = true;
}

inline void delete_entity_operation::undo() {
    auto& world            = engine_->get_world();
    auto& hierarchy_sys = world.template system<gfx::hierarchy_system>();
    auto& transform_sys = world.template system<gfx::transform_system>();

    auto modifier = world.create()
        .template with<gfx::hierarchy_component>()
        .template with<gfx::transform_component>()
        .template with<gfx::spatial_component>();

    const auto ent = modifier.get_entity();

    transform_sys.modify(ent).set_transform(transform_);

    if (with_model_) {
        modifier.template with<gfx::model_component>();

        auto& model_sys = world.template system<gfx::model_system>();
        model_sys.modify(ent).set_model(saved_model_);
    }

    if (!parent_name_.empty()) {
        auto parent_ent = state_->scene.name_to_entity[parent_name_];
        hierarchy_sys.modify(ent).set_parent(parent_ent);
    }

    state_->scene.name_to_entity[params_.name] = ent;
    state_->scene.entity_to_name[ent]          = params_.name;

    if (state_->scene.root_name.empty()) {
        state_->scene.root_name = params_.name;
    }
    state_->scene.selected_name       = params_.name;
    state_->file.has_unsaved_changes = true;

    state_->scene.entities.push_back(ent);
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_DELETE_ENTITY_OPERATION_INL_H
