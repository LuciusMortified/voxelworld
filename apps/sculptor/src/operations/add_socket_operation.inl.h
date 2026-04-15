#pragma once

#ifndef VW_SCULPTOR_ADD_SOCKET_OPERATION_INL_H
#define VW_SCULPTOR_ADD_SOCKET_OPERATION_INL_H

namespace vw::sculptor {

inline add_socket_operation::add_socket_operation(
    engine_type& engine, app_state& st, const add_socket_params& params
)
    : engine_(&engine), state_(&st), params_(params) {}

inline void add_socket_operation::execute() {
    auto& world         = engine_->get_world();
    auto& socket_sys = world.template get_system<gfx::socket_system>();

    const auto ent = state_->scene.name_to_entity[params_.entity_name];
    socket_sys  //
        .modify(ent)
        .add_socket(params_.socket_name, params_.position, params_.rotation, params_.scale);
    state_->file.has_unsaved_changes = true;
}

inline void add_socket_operation::undo() {
    auto& world         = engine_->get_world();
    auto& socket_sys = world.template get_system<gfx::socket_system>();

    const auto ent  = state_->scene.name_to_entity[params_.entity_name];
    const auto pkey = socket_state::socket_preview_key(params_.entity_name, params_.socket_name);
    state_->sockets.socket_previews.erase(pkey);
    socket_sys  //
        .modify(ent)
        .remove_socket(params_.socket_name);
    state_->file.has_unsaved_changes = true;
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_ADD_SOCKET_OPERATION_INL_H
