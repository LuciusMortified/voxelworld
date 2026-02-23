#pragma once

#ifndef VW_SCULPTOR_MODIFY_KEYFRAME_OPERATION_INL_H
#define VW_SCULPTOR_MODIFY_KEYFRAME_OPERATION_INL_H

namespace vw::sculptor {

inline modify_keyframe_operation::modify_keyframe_operation(
    engine_type& engine, app_state& state, const modify_keyframe_params& params
)
    : base_operation(), engine_(&engine), state_(&state), params_(params) {}

inline void modify_keyframe_operation::execute() {
    apply(params_.new_keyframe);
}

inline void modify_keyframe_operation::undo() {
    apply(params_.old_keyframe);
}

inline void modify_keyframe_operation::apply(
    const keyframe_value& replacement
) const {
    const auto& registry = engine_->get_world().get_animation_clip_registry();
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

    const uint32 id =
        std::visit([](const auto& kf) -> uint32 { return kf.id(); }, params_.old_keyframe);

    if (params_.property == gfx::animation_property::rotation) {
        auto& channel = std::get<gfx::animation_channel<quat>>(*channel_var);
        channel.replace(id, std::get<gfx::keyframe_quat>(replacement));
    } else {
        auto& channel = std::get<gfx::animation_channel<vec3f>>(*channel_var);
        channel.replace(id, std::get<gfx::keyframe_vec3f>(replacement));
    }

    track->mark_dirty();
    state_->anim.unsaved_clips[params_.clip_name] = true;
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_MODIFY_KEYFRAME_OPERATION_INL_H
