#pragma once

#ifndef VW_SCULPTOR_MODIFY_KEYFRAME_OPERATION_INL_H
#define VW_SCULPTOR_MODIFY_KEYFRAME_OPERATION_INL_H

namespace vw::sculptor {

inline modify_keyframe_operation::modify_keyframe_operation(
    engine_type& engine, app_state& state, const modify_keyframe_params& params
)
    : base_operation(), engine_(&engine), state_(&state), params_(params) {}

inline void modify_keyframe_operation::execute() {
    apply(params_.old_keyframe, params_.new_keyframe);
}

inline void modify_keyframe_operation::undo() {
    apply(params_.new_keyframe, params_.old_keyframe);
}

inline void modify_keyframe_operation::apply(
    const keyframe_value& remove, const keyframe_value& add
) {
    auto& registry = engine_->get_world().get_animation_clip_registry();
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

    float32 remove_time = std::visit(
        [](const auto& kf) -> float32 { return kf.time; }, remove
    );

    std::visit(
        [remove_time](auto& channel) {
            auto& kfs = channel.get_keyframes_mut();
            auto it   = std::find_if(kfs.begin(), kfs.end(), [remove_time](const auto& kf) {
                return std::abs(kf.time - remove_time) < 0.0001f;
            });
            if (it != kfs.end()) {
                kfs.erase(it);
            }
        },
        *channel_var
    );

    if (params_.property == gfx::animation_property::rotation) {
        auto& channel = std::get<gfx::animation_channel<quat>>(*channel_var);
        channel.add(std::get<gfx::keyframe_quat>(add));
    } else {
        auto& channel = std::get<gfx::animation_channel<vec3f>>(*channel_var);
        channel.add(std::get<gfx::keyframe_vec3f>(add));
    }

    track->mark_dirty();
    state_->has_unsaved_changes               = true;
    state_->unsaved_clips[params_.clip_name] = true;
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_MODIFY_KEYFRAME_OPERATION_INL_H
