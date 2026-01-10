#pragma once

#ifndef VW_SCULPTOR_DELETE_ENTITY_OPERATION_INL_H
#define VW_SCULPTOR_DELETE_ENTITY_OPERATION_INL_H

namespace vw::sculptor {

template <typename WC>
delete_entity_operation<WC>::delete_entity_operation(
    engine_type& engine, app_state& state, const delete_entity_params& params
)
    : engine_(&engine), state_(&state), params_(params) {}

template <typename WC>
void delete_entity_operation<WC>::execute() {
    auto ent = state_->name_to_entity[params_.name];

    auto& world          = engine_->get_world();
    auto& hierarchy_comp = world.template get_component<gfx::hierarchy_component>(ent);
    auto& transform_comp = world.template get_component<gfx::transform_component>(ent);

    bool has_parent = hierarchy_comp.has_parent();
    if (has_parent) {
        parent_name_ = state_->entity_to_name[hierarchy_comp.get_parent()];
    }

    transform_ = transform_comp.get_transform();

    if (world.template has_component<gfx::model_component>(ent)) {
        auto& model_comp = world.template get_component<gfx::model_component>(ent);

        if (model_comp.has_model()) {
            with_model_ = true;
            size_       = model_comp.size();
            voxels_     = model_comp.get_voxels();
        }
    }

    world.destroy_entity(ent);
    state_->entity_to_name.erase(ent);
    state_->name_to_entity.erase(params_.name);

    if (state_->root_name == params_.name) {
        state_->root_name = "";
    }
    if (has_parent) {
        state_->selected_name = parent_name_;
    } else {
        state_->selected_name = "";
    }
}

template <typename WC>
void delete_entity_operation<WC>::undo() {
    auto& world            = engine_->get_world();
    auto& hierarchy_system = world.get_hierarchy_system();
    auto& transform_system = world.get_transform_system();

    auto ent = world.create_entity();
    world.template add_component<gfx::hierarchy_component>(ent);
    world.template add_component<gfx::transform_component>(ent);
    world.template add_component<gfx::spatial_component>(ent);

    transform_system.modify(ent).set_transform(transform_);

    if (with_model_) {
        world.template add_component<gfx::model_component>(ent);

        auto& model_registry = world.get_model_registry();
        auto& model_system   = world.get_model_system();

        auto model = model_registry.create(params_.name, size_);
        model->set_voxels(voxels_);
        model_system.modify(ent).set_model(model);
    }

    if (!parent_name_.empty()) {
        auto parent_ent = state_->name_to_entity[parent_name_];
        hierarchy_system.modify(ent).set_parent(parent_ent);
    }

    state_->name_to_entity[params_.name] = ent;
    state_->entity_to_name[ent]          = params_.name;

    if (state_->root_name.empty()) {
        state_->root_name = params_.name;
    }
    state_->selected_name = params_.name;
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_DELETE_ENTITY_OPERATION_INL_H
