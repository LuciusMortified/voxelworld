#pragma once

#ifndef VW_SCULPTOR_SET_SOCKET_TRANSFORM_OPERATION_INL_H
#define VW_SCULPTOR_SET_SOCKET_TRANSFORM_OPERATION_INL_H

namespace vw::sculptor {

inline set_socket_transform_operation::set_socket_transform_operation(
    engine_type& engine, app_state& st, const set_socket_transform_params& params
)
    : engine_(&engine), state_(&st), params_(params) {}

inline void set_socket_transform_operation::execute() {
    auto& world = engine_->get_world();
    auto ent    = state_->scene.name_to_entity[params_.entity_name];

    auto& socket_comp = world.template get<gfx::socket_component>(ent);
    const auto* sp    = socket_comp.find(params_.socket_name);
    if (!sp) {
        return;
    }

    previous_position_ = sp->position;
    previous_rotation_ = sp->rotation;
    previous_scale_    = sp->scale;

    auto& sockets = const_cast<std::vector<gfx::socket_point>&>(socket_comp.get_sockets());
    for (auto& slot : sockets) {
        if (slot.name == params_.socket_name) {
            slot.position = params_.position;
            slot.rotation = params_.rotation;
            slot.scale    = params_.scale;
            break;
        }
    }

    update_attached_(params_.position, params_.rotation, params_.scale);
    update_preview_(params_.position, params_.rotation, params_.scale);
    state_->file.has_unsaved_changes = true;
}

inline void set_socket_transform_operation::undo() {
    auto& world = engine_->get_world();
    auto ent    = state_->scene.name_to_entity[params_.entity_name];

    auto& socket_comp = world.template get<gfx::socket_component>(ent);
    auto& sockets     = const_cast<std::vector<gfx::socket_point>&>(socket_comp.get_sockets());
    for (auto& slot : sockets) {
        if (slot.name == params_.socket_name) {
            slot.position = previous_position_;
            slot.rotation = previous_rotation_;
            slot.scale    = previous_scale_;
            break;
        }
    }

    update_attached_(previous_position_, previous_rotation_, previous_scale_);
    update_preview_(previous_position_, previous_rotation_, previous_scale_);
    state_->file.has_unsaved_changes = true;
}

inline void set_socket_transform_operation::update_attached_(
    const vec3f& position, const quat& rotation, const vec3f& scale
) {
    auto& world = engine_->get_world();
    auto ent    = state_->scene.name_to_entity[params_.entity_name];

    auto& socket_comp = world.template get<gfx::socket_component>(ent);
    const auto* sp    = socket_comp.find(params_.socket_name);
    if (!sp || !sp->attached.is_valid()) {
        return;
    }

    auto& transform_sys = world.template system<gfx::transform_system>();
    transform_sys.modify(sp->attached)
        .set_position(position)
        .set_rotation(rotation)
        .set_scale(scale);
}

inline void set_socket_transform_operation::update_preview_(
    const vec3f& position, const quat& rotation, const vec3f& scale
) {
    const auto pkey = socket_state::socket_preview_key(params_.entity_name, params_.socket_name);
    const auto it   = state_->sockets.socket_previews.find(pkey);
    if (it == state_->sockets.socket_previews.end()) {
        return;
    }

    const auto& preview = it->second;
    if (preview.entities.empty()) {
        return;
    }

    auto& transform_sys = engine_->get_world().template system<gfx::transform_system>();
    transform_sys.modify(preview.entities[0])
        .set_position(position)
        .set_rotation(rotation)
        .set_scale(scale);
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_SET_SOCKET_TRANSFORM_OPERATION_INL_H
