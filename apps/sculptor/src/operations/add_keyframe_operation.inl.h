#pragma once

#ifndef VW_SCULPTOR_ADD_KEYFRAME_OPERATION_INL_H
#define VW_SCULPTOR_ADD_KEYFRAME_OPERATION_INL_H

namespace vw::sculptor {

inline add_keyframe_operation::add_keyframe_operation(
    engine_type& engine, app_state& state, const add_keyframe_params& params
)
    : base_operation(), engine_(&engine), state_(&state), params_(params) {}

inline void add_keyframe_operation::execute() {
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
        if (params_.property == gfx::animation_property::rotation) {
            auto channel = gfx::make_animation_channel<gfx::animation_property::rotation>();
            channel.add(std::get<gfx::keyframe_quat>(params_.keyframe));
            track->add<gfx::animation_property::rotation>(std::move(channel));
        } else if (params_.property == gfx::animation_property::position) {
            auto channel = gfx::make_animation_channel<gfx::animation_property::position>();
            channel.add(std::get<gfx::keyframe_vec3f>(params_.keyframe));
            track->add<gfx::animation_property::position>(std::move(channel));
        } else if (params_.property == gfx::animation_property::scale) {
            auto channel = gfx::make_animation_channel<gfx::animation_property::scale>();
            channel.add(std::get<gfx::keyframe_vec3f>(params_.keyframe));
            track->add<gfx::animation_property::scale>(std::move(channel));
        } else if (params_.property == gfx::animation_property::origin) {
            auto channel = gfx::make_animation_channel<gfx::animation_property::origin>();
            channel.add(std::get<gfx::keyframe_vec3f>(params_.keyframe));
            track->add<gfx::animation_property::origin>(std::move(channel));
        }
        created_channel_ = true;
    } else {
        if (params_.property == gfx::animation_property::rotation) {
            auto& channel = std::get<gfx::animation_channel<quat>>(*channel_var);
            channel.add(std::get<gfx::keyframe_quat>(params_.keyframe));
        } else {
            auto& channel = std::get<gfx::animation_channel<vec3f>>(*channel_var);
            channel.add(std::get<gfx::keyframe_vec3f>(params_.keyframe));
        }
        created_channel_ = false;
    }

    track->mark_dirty();
    state_->has_unsaved_changes               = true;
    state_->unsaved_clips[params_.clip_name] = true;
}

inline void add_keyframe_operation::undo() {
    auto& registry = engine_->get_world().get_animation_clip_registry();
    auto clip      = registry.get(params_.clip_name);
    if (!clip) {
        return;
    }

    auto* track = clip->get_track_mut(params_.track_name);
    if (!track) {
        return;
    }

    if (created_channel_) {
        track->remove_channel(params_.property);
    } else {
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
    }

    track->mark_dirty();
    state_->has_unsaved_changes               = true;
    state_->unsaved_clips[params_.clip_name] = true;
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_ADD_KEYFRAME_OPERATION_INL_H
