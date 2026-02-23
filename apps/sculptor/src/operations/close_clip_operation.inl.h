#pragma once

#ifndef VW_SCULPTOR_CLOSE_CLIP_OPERATION_INL_H
#define VW_SCULPTOR_CLOSE_CLIP_OPERATION_INL_H

namespace vw::sculptor {

inline close_clip_operation::close_clip_operation(
    engine_type& engine, app_state& state, const close_clip_params& params
)
    : base_operation(), engine_(&engine), state_(&state), params_(params) {}

inline void close_clip_operation::execute() {
    auto& registry = engine_->get_world().get_animation_clip_registry();
    saved_clip_    = registry.get(params_.name);
    registry.remove(params_.name);

    if (state_->selected_clip_name == params_.name) {
        state_->selected_clip_name.clear();
        state_->selected_track_name.clear();
        state_->selected_keyframe_id = gfx::invalid_keyframe_id;
    }
    state_->unsaved_clips.erase(params_.name);
}

inline void close_clip_operation::undo() {
    auto& registry = engine_->get_world().get_animation_clip_registry();
    registry.add(params_.name, saved_clip_);
    state_->selected_clip_name = params_.name;
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_CLOSE_CLIP_OPERATION_INL_H
