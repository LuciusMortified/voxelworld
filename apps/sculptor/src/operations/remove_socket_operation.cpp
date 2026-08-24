module vw.sculptor;

import std;

import vw.core;
import vw.ecs;
import vw.world;
import vw.platform;
import vw.gfx;

namespace vw::sculptor {

remove_socket_operation::remove_socket_operation(
    engine_type& engine, app_state& st, const remove_socket_params& params
)
    : engine_(&engine), state_(&st), params_(params) {}

auto remove_socket_operation::execute() -> void {
    auto& world         = engine_->get_world();
    auto& socket_sys = world.system<ecs::socket_system>();

    auto ent = state_->scene.name_to_entity[params_.entity_name];

    auto& socket_comp   = world.get<ecs::socket_component>(ent);
    const auto* sp      = socket_comp.find(params_.socket_name);
    if (sp) {
        saved_position_ = sp->position;
        saved_rotation_ = sp->rotation;
        saved_scale_    = sp->scale;
    }

    const auto pkey = socket_state::socket_preview_key(params_.entity_name, params_.socket_name);
    state_->sockets.erase_preview(pkey, world);
    socket_sys.modify(ent).remove_socket(params_.socket_name);
    state_->file.has_unsaved_changes = true;
}

auto remove_socket_operation::undo() -> void {
    auto& world         = engine_->get_world();
    auto& socket_sys = world.system<ecs::socket_system>();

    auto ent = state_->scene.name_to_entity[params_.entity_name];
    socket_sys.modify(ent).add_socket(params_.socket_name, saved_position_, saved_rotation_,
                                            saved_scale_);
    state_->file.has_unsaved_changes = true;
}

}  // namespace vw::sculptor
