module vw.sculptor;

import std;

import vw.core;
import vw.ecs;
import vw.world;
import vw.platform;
import vw.gfx;

namespace vw::sculptor {

add_socket_component_operation::add_socket_component_operation(
    engine_type& engine, app_state& state, const add_socket_component_params& params
)
    : engine_(&engine), state_(&state), params_(params) {}

auto add_socket_component_operation::execute() -> void {
    if (!state_->scene.name_to_entity.contains(params_.name)) {
        return;
    }
    const auto ent = state_->scene.name_to_entity[params_.name];

    engine_->get_world().modify(ent).with<ecs::socket_component>();
    state_->file.has_unsaved_changes = true;
}

auto add_socket_component_operation::undo() -> void {
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
