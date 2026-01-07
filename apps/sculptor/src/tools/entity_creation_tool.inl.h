#pragma once

#ifndef VW_SCULPTOR_ENTITY_CREATION_TOOL_INL_H
#define VW_SCULPTOR_ENTITY_CREATION_TOOL_INL_H

namespace vw::sculptor {

template <typename WC>
entity_creation_tool<WC>::entity_creation_tool(
engine_type& eng, state& st
)
    : engine_{&eng}, state_{&st} {}

template <typename WC>
void entity_creation_tool<WC>::execute(
    const creation_params& params
) {
    auto& world            = engine_->get_world();
    auto& model_registry   = world.get_model_registry();
    auto& model_system     = world.get_model_system();
    auto& hierarchy_system = world.get_hierarchy_system();

    auto ent = world.create_entity();
    world.template add_component<gfx::hierarchy_component>(ent);
    world.template add_component<gfx::transform_component>(ent);
    world.template add_component<gfx::model_component>(ent);
    world.template add_component<gfx::spatial_component>(ent);

    auto model = model_registry.create(
        params.name, params.model_size.x, params.model_size.y, params.model_size.z
    );
    model->fill(voxel{state_->selected_color});
    model_system.modify(ent).set_model(model);

    if (params.parent.is_valid()) {
        hierarchy_system.modify(ent).set_parent(params.parent);
    } else {
        state_->root_entity = ent;
    }

    state_->entity_map[ent] = {
        .name       = std::string(params.name),
        .parent     = params.parent,
        .depth      = hierarchy_system.get_hierarchy_depth(ent),
        .model_size = params.model_size,
    };
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_ENTITY_CREATION_TOOL_INL_H
