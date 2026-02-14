#pragma once

#ifndef VW_SCULPTOR_REMOVE_KEYFRAME_OPERATION_INL_H
#define VW_SCULPTOR_REMOVE_KEYFRAME_OPERATION_INL_H

namespace vw::sculptor {

inline remove_keyframe_operation::remove_keyframe_operation(
    engine_type& engine, app_state& state, const remove_keyframe_params& params
)
    : base_operation(), engine_(&engine), state_(&state), params_(params) {}

inline void remove_keyframe_operation::execute() {
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

    float32 time = std::visit(
        [](const auto& kf) -> float32 { return kf.time; }, params_.keyframe
    );

    std::visit(
        [time](auto& channel) {
            auto& kfs = channel.get_keyframes_mut();
            std::erase_if(kfs, [time](const auto& kf) {
                return std::abs(kf.time - time) < 0.0001f;
            });
        },
        *channel_var
    );

    track->mark_dirty();
    state_->selected_keyframe_time = -1.f;
    state_->has_unsaved_changes    = true;
}

inline void remove_keyframe_operation::undo() {
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

    if (params_.property == gfx::animation_property::rotation) {
        auto& channel = std::get<gfx::animation_channel<quat>>(*channel_var);
        channel.add(std::get<gfx::keyframe_quat>(params_.keyframe));
    } else {
        auto& channel = std::get<gfx::animation_channel<vec3f>>(*channel_var);
        channel.add(std::get<gfx::keyframe_vec3f>(params_.keyframe));
    }

    track->mark_dirty();
    state_->has_unsaved_changes = true;
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_REMOVE_KEYFRAME_OPERATION_INL_H
