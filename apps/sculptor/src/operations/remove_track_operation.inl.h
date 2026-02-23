#pragma once

#ifndef VW_SCULPTOR_REMOVE_TRACK_OPERATION_INL_H
#define VW_SCULPTOR_REMOVE_TRACK_OPERATION_INL_H

namespace vw::sculptor {

inline remove_track_operation::remove_track_operation(
    engine_type& eng, app_state& state, remove_track_params params
)
    : engine_(&eng), state_(&state), params_(std::move(params)) {}

inline void remove_track_operation::execute() {
    auto& registry = engine_->get_world().get_animation_clip_registry();
    auto clip      = registry.get(params_.clip_name);
    if (!clip) {
        return;
    }

    auto* track = clip->get_track(params_.track_name);
    if (track) {
        saved_track_ = *track;
    }

    clip->remove_track(params_.track_name);

    if (state_->selected_track_name == params_.track_name) {
        state_->selected_track_name.clear();
        state_->selected_keyframe_id = gfx::invalid_keyframe_id;
    }
    state_->expanded_tracks.erase(params_.track_name);
}

inline void remove_track_operation::undo() {
    auto& registry = engine_->get_world().get_animation_clip_registry();
    auto clip      = registry.get(params_.clip_name);
    if (!clip || !saved_track_) {
        return;
    }
    clip->add_track(*saved_track_);
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_REMOVE_TRACK_OPERATION_INL_H
