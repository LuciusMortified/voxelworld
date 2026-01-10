#pragma once

#ifndef VW_SCULPTOR_CREATE_ENTITY_OPERATION_INL_H
#define VW_SCULPTOR_CREATE_ENTITY_OPERATION_INL_H

namespace vw::sculptor {

template <typename WC>
create_entity_operation<WC>::create_entity_operation(
    engine_type& engine, app_state& state, const create_entity_params& params
)
    : base_operation(), engine_(&engine), state_(&state), params_(params) {}

template <typename WC>
void create_entity_operation<WC>::execute() {
    auto& world            = engine_->get_world();
    auto& hierarchy_system = world.get_hierarchy_system();
    auto& transform_system = world.get_transform_system();

    auto ent = world.create_entity();
    world.template add_component<gfx::hierarchy_component>(ent);
    world.template add_component<gfx::transform_component>(ent);
    world.template add_component<gfx::spatial_component>(ent);

    transform_system.modify(ent).set_origin(
        vec3f{-params_.size.x / 2.f, -params_.size.y / 2.f, -params_.size.z / 2.f}
    );

    if (params_.with_model) {
        world.template add_component<gfx::model_component>(ent);

        auto& model_registry = world.get_model_registry();
        auto& model_system   = world.get_model_system();

        auto model = model_registry.create(params_.name, params_.size);
        model->fill(voxel{state_->selected_color});
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
}

template <typename WC>
void create_entity_operation<WC>::undo() {
    auto ent    = state_->name_to_entity[params_.name];
    auto& world = engine_->get_world();
    world.destroy_entity(ent);
    if (state_->root_name == params_.name) {
        state_->root_name = "";
    }
    state_->selected_name = "";
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_CREATE_ENTITY_OPERATION_INL_H
