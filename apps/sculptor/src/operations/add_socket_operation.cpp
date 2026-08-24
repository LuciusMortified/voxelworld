module vw.sculptor;

import std;

import vw.core;
import vw.ecs;
import vw.world;
import vw.platform;
import vw.gfx;

namespace vw::sculptor {

add_socket_operation::add_socket_operation(
    engine_type& engine, app_state& st, const add_socket_params& params
)
    : engine_(&engine), state_(&st), params_(params) {}

auto add_socket_operation::execute() -> void {
    auto& world         = engine_->get_world();
    auto& socket_sys = world.system<ecs::socket_system>();

    const auto ent = state_->scene.name_to_entity[params_.entity_name];
    socket_sys  //
        .modify(ent)
        .add_socket(params_.socket_name, params_.position, params_.rotation, params_.scale);
    state_->file.has_unsaved_changes = true;
}

auto add_socket_operation::undo() -> void {
    auto& world         = engine_->get_world();
    auto& socket_sys = world.system<ecs::socket_system>();

    const auto ent  = state_->scene.name_to_entity[params_.entity_name];
    const auto pkey = socket_state::socket_preview_key(params_.entity_name, params_.socket_name);
    state_->sockets.erase_preview(pkey, world);
    socket_sys  //
        .modify(ent)
        .remove_socket(params_.socket_name);
    state_->file.has_unsaved_changes = true;
}

}  // namespace vw::sculptor
