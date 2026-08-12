module vw.sculptor;

import std;

import vw.core;
import vw.ecs;
import vw.world;
import vw.platform;
import vw.gfx;

namespace vw::sculptor {

remove_socket_component_operation::remove_socket_component_operation(
    engine_type& engine, app_state& state, const remove_socket_component_params& params
)
    : engine_(&engine), state_(&state), params_(params) {}

void remove_socket_component_operation::execute() {
    auto& world = engine_->get_world();
    if (!state_->scene.name_to_entity.contains(params_.name)) {
        return;
    }
    const auto ent = state_->scene.name_to_entity[params_.name];

    const auto& socket_comp = world.get<ecs::socket_component>(ent);
    saved_sockets_.clear();
    for (const auto& sp : socket_comp.get_sockets()) {
        saved_sockets_.push_back({sp.name, sp.position, sp.rotation, sp.scale});
    }

    state_->sockets.erase_previews_for(params_.name, world);

    world.modify(ent).without<ecs::socket_component>();
    state_->file.has_unsaved_changes = true;
}

void remove_socket_component_operation::undo() {
    auto& world         = engine_->get_world();
    auto& socket_sys = world.system<ecs::socket_system>();

    if (!state_->scene.name_to_entity.contains(params_.name)) {
        return;
    }
    const auto ent = state_->scene.name_to_entity[params_.name];

    world.modify(ent).with<ecs::socket_component>();

    for (const auto& ss : saved_sockets_) {
        socket_sys.modify(ent).add_socket(ss.name, ss.position, ss.rotation, ss.scale);
    }
    state_->file.has_unsaved_changes = true;
}

}  // namespace vw::sculptor
