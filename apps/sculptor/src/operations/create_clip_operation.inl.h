#pragma once

#ifndef VW_SCULPTOR_CREATE_CLIP_OPERATION_INL_H
#define VW_SCULPTOR_CREATE_CLIP_OPERATION_INL_H

namespace vw::sculptor {

inline create_clip_operation::create_clip_operation(
    engine_type& engine, app_state& state, const create_clip_params& params
)
    : base_operation(), engine_(&engine), state_(&state), params_(params) {}

inline void create_clip_operation::execute() {
    auto& registry = engine_->get_world().template get_resource<asset::animation_clip_registry>();
    (void)registry.create(params_.name);
    state_->anim.selected_clip_name          = params_.name;
    state_->ui.show_timeline                 = true;
    state_->anim.unsaved_clips[params_.name] = true;
}

inline void create_clip_operation::undo() {
    auto& registry = engine_->get_world().template get_resource<asset::animation_clip_registry>();
    registry.remove(params_.name);
    state_->anim.selected_clip_name.clear();
    state_->anim.unsaved_clips.erase(params_.name);
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_CREATE_CLIP_OPERATION_INL_H
