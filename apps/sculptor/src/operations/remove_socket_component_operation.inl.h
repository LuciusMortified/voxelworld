#pragma once

#ifndef VW_SCULPTOR_REMOVE_SOCKET_COMPONENT_OPERATION_INL_H
#define VW_SCULPTOR_REMOVE_SOCKET_COMPONENT_OPERATION_INL_H

namespace vw::sculptor {

inline remove_socket_component_operation::remove_socket_component_operation(
    engine_type& engine, app_state& state, const remove_socket_component_params& params
)
    : engine_(&engine), state_(&state), params_(params) {}

inline void remove_socket_component_operation::execute() {
    auto& world = engine_->get_world();
    if (!state_->scene.name_to_entity.contains(params_.name)) {
        return;
    }
    const auto ent = state_->scene.name_to_entity[params_.name];

    const auto& socket_comp = world.get<gfx::socket_component>(ent);
    saved_sockets_.clear();
    for (const auto& sp : socket_comp.get_sockets()) {
        saved_sockets_.push_back({sp.name, sp.position, sp.rotation, sp.scale});
    }

    state_->sockets.erase_previews_for(params_.name, world);

    world.modify(ent).without<gfx::socket_component>();
    state_->file.has_unsaved_changes = true;
}

inline void remove_socket_component_operation::undo() {
    auto& world         = engine_->get_world();
    auto& socket_sys = world.system<gfx::socket_system>();

    if (!state_->scene.name_to_entity.contains(params_.name)) {
        return;
    }
    const auto ent = state_->scene.name_to_entity[params_.name];

    world.modify(ent).with<gfx::socket_component>();

    for (const auto& ss : saved_sockets_) {
        socket_sys.modify(ent).add_socket(ss.name, ss.position, ss.rotation, ss.scale);
    }
    state_->file.has_unsaved_changes = true;
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_REMOVE_SOCKET_COMPONENT_OPERATION_INL_H
