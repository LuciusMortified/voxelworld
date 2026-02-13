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
    auto& hierarchy_system = world.get_hierarchy_system();
    auto& transform_system = world.get_transform_system();
    auto& model_registry   = world.get_model_registry();
    auto& model_system     = world.get_model_system();

    auto ent_guard = std::make_unique<gfx::entity_guard<>>(world);
    ent_guard->with<gfx::hierarchy_component>();
    ent_guard->with<gfx::transform_component>();
    ent_guard->with<gfx::spatial_component>();

    std::shared_ptr<gfx::model> model = nullptr;
    if (params_.with_model) {
        ent_guard->with<gfx::model_component>();

        model = model_registry.create(params_.name, params_.size);
        model->fill(voxel{state_->selected_color});
    }

    auto ent = ent_guard->get_entity();

    transform_system.modify(ent)
        .set_origin(vec3f{-params_.size.x / 2.f, -params_.size.y / 2.f, -params_.size.z / 2.f});

    if (model) {
        model_system.modify(ent).set_model(model);
    }

    if (!params_.parent_name.empty() && state_->name_to_entity.contains(params_.parent_name)) {
        auto parent_ent = state_->name_to_entity[params_.parent_name];
        hierarchy_system.modify(ent).set_parent(parent_ent);
    }

    state_->name_to_entity[params_.name] = ent;
    state_->entity_to_name[ent]          = params_.name;

    if (state_->root_name.empty()) {
        state_->root_name = params_.name;
    }
    state_->selected_name = params_.name;

    state_->entities.push_back(std::move(ent_guard));
    state_->has_unsaved_changes = true;
}

inline void create_entity_operation::undo() {
    auto ent = state_->name_to_entity[params_.name];

    state_->entity_to_name.erase(ent);
    state_->name_to_entity.erase(params_.name);

    state_->entities.erase(
        std::remove_if(
            state_->entities.begin(),
            state_->entities.end(),
            [ent](const auto& guard) { return guard->get_entity() == ent; }
        ),
        state_->entities.end()
    );

    if (state_->root_name == params_.name) {
        state_->root_name = "";
    }
    state_->selected_name       = "";
    state_->has_unsaved_changes = true;
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_CREATE_ENTITY_OPERATION_INL_H
