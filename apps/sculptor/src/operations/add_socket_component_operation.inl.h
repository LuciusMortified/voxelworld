#pragma once

#ifndef VW_SCULPTOR_ADD_SOCKET_COMPONENT_OPERATION_INL_H
#define VW_SCULPTOR_ADD_SOCKET_COMPONENT_OPERATION_INL_H

namespace vw::sculptor {

inline add_socket_component_operation::add_socket_component_operation(
    engine_type& engine, app_state& state, const add_socket_component_params& params
)
    : engine_(&engine), state_(&state), params_(params) {}

inline void add_socket_component_operation::execute() {
    if (!state_->scene.name_to_entity.contains(params_.name)) {
        return;
    }
    const auto ent = state_->scene.name_to_entity[params_.name];

    engine_->get_world().modify(ent).with<ecs::socket_component>();
    state_->file.has_unsaved_changes = true;
}

inline void add_socket_component_operation::undo() {
    if (!state_->scene.name_to_entity.contains(params_.name)) {
        return;
    }
    const auto ent = state_->scene.name_to_entity[params_.name];

    auto& world = engine_->get_world();
    state_->sockets.erase_previews_for(params_.name, world);

    world.modify(ent).without<ecs::socket_component>();
    state_->file.has_unsaved_changes = true;
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_ADD_SOCKET_COMPONENT_OPERATION_INL_H
