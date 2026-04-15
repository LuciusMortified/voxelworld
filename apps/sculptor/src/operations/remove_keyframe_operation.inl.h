#pragma once

#ifndef VW_SCULPTOR_REMOVE_KEYFRAME_OPERATION_INL_H
#define VW_SCULPTOR_REMOVE_KEYFRAME_OPERATION_INL_H

namespace vw::sculptor {

inline remove_keyframe_operation::remove_keyframe_operation(
    engine_type& engine, app_state& state, const remove_keyframe_params& params
)
    : base_operation(), engine_(&engine), state_(&state), params_(params) {}

inline void remove_keyframe_operation::execute() {
    auto& registry = engine_->get_world().template get_resource<gfx::animation_clip_registry>();
    auto clip      = registry.get(params_.clip_name);
    if (!clip) {
        return;
    }

    auto* track = clip->get_track_mut(params_.track_name);
    if (!track) {
        return;
    }

    auto* channel_var = track->get_channel_mut(params_.property);
    if (!channel_var) {
        return;
    }

    uint32 remove_id =
        std::visit([](const auto& kf) -> uint32 { return kf.id(); }, params_.keyframe);

    std::visit([remove_id](auto& channel) { channel.remove(remove_id); }, *channel_var);

    track->mark_dirty();
    state_->anim.need_apply_pose                  = true;
    state_->anim.selected_keyframe_id             = gfx::invalid_keyframe_id;
    state_->anim.unsaved_clips[params_.clip_name] = true;
}

inline void remove_keyframe_operation::undo() {
    const auto& registry = engine_->get_world().template get_resource<gfx::animation_clip_registry>();
    const auto clip      = registry.get(params_.clip_name);
    if (!clip) {
        return;
    }

    auto* track = clip->get_track_mut(params_.track_name);
    if (!track) {
        return;
    }

    auto* channel_var = track->get_channel_mut(params_.property);
    if (!channel_var) {
        return;
    }

    if (params_.property == gfx::animation_property::rotation) {
        auto& channel = std::get<gfx::animation_channel<quat>>(*channel_var);
        channel.add(std::get<gfx::keyframe_quat>(params_.keyframe));
    } else {
        auto& channel = std::get<gfx::animation_channel<vec3f>>(*channel_var);
        channel.add(std::get<gfx::keyframe_vec3f>(params_.keyframe));
    }

    track->mark_dirty();
    state_->anim.need_apply_pose                  = true;
    state_->anim.unsaved_clips[params_.clip_name] = true;
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_REMOVE_KEYFRAME_OPERATION_INL_H
