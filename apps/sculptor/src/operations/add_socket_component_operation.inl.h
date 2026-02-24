#pragma once

#ifndef VW_SCULPTOR_ADD_SOCKET_COMPONENT_OPERATION_INL_H
#define VW_SCULPTOR_ADD_SOCKET_COMPONENT_OPERATION_INL_H

namespace vw::sculptor {

inline add_socket_component_operation::add_socket_component_operation(
    engine_type& engine, app_state& state, const add_socket_component_params& params
)
    : engine_(&engine), state_(&state), params_(params) {}

inline void add_socket_component_operation::execute() {
    const auto ent = state_->scene.name_to_entity[params_.name];
    auto* guard    = state_->scene.find_guard(ent);
    if (!guard) {
        return;
    }

    guard->with<gfx::socket_component>();
    state_->file.has_unsaved_changes = true;
}

inline void add_socket_component_operation::undo() {
    const auto ent = state_->scene.name_to_entity[params_.name];
    auto* guard    = state_->scene.find_guard(ent);
    if (!guard) {
        return;
    }

    const auto& entity_name = params_.name;
    std::erase_if(state_->sockets.socket_previews, [&entity_name](const auto& kv) {
        return kv.first.starts_with(entity_name + ":");
    });

    guard->without<gfx::socket_component>();
    state_->file.has_unsaved_changes = true;
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_ADD_SOCKET_COMPONENT_OPERATION_INL_H
