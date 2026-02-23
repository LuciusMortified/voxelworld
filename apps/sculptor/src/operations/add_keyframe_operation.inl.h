#pragma once

#ifndef VW_SCULPTOR_ADD_KEYFRAME_OPERATION_INL_H
#define VW_SCULPTOR_ADD_KEYFRAME_OPERATION_INL_H

namespace vw::sculptor {

inline add_keyframe_operation::add_keyframe_operation(
    engine_type& engine, app_state& state, const add_keyframe_params& params
)
    : engine_(&engine), state_(&state), params_(params) {}

inline void add_keyframe_operation::execute() {
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
        if (params_.property == gfx::animation_property::rotation) {
            auto channel = gfx::make_animation_channel<gfx::animation_property::rotation>();
            channel.add(std::get<gfx::keyframe_quat>(params_.keyframe));
            track->template add<gfx::animation_property::rotation>(std::move(channel));
        } else if (params_.property == gfx::animation_property::position) {
            auto channel = gfx::make_animation_channel<gfx::animation_property::position>();
            channel.add(std::get<gfx::keyframe_vec3f>(params_.keyframe));
            track->template add<gfx::animation_property::position>(std::move(channel));
        } else if (params_.property == gfx::animation_property::scale) {
            auto channel = gfx::make_animation_channel<gfx::animation_property::scale>();
            channel.add(std::get<gfx::keyframe_vec3f>(params_.keyframe));
            track->template add<gfx::animation_property::scale>(std::move(channel));
        } else if (params_.property == gfx::animation_property::origin) {
            auto channel = gfx::make_animation_channel<gfx::animation_property::origin>();
            channel.add(std::get<gfx::keyframe_vec3f>(params_.keyframe));
            track->template add<gfx::animation_property::origin>(std::move(channel));
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
    state_->anim.unsaved_clips[params_.clip_name] = true;
}

inline void add_keyframe_operation::undo() {
    const auto& registry = engine_->get_world().get_animation_clip_registry();
    const auto clip      = registry.get(params_.clip_name);
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

        uint32 remove_id =
            std::visit([](const auto& kf) -> uint32 { return kf.id(); }, params_.keyframe);

        std::visit([remove_id](auto& channel) { channel.remove(remove_id); }, *channel_var);
    }

    track->mark_dirty();
    state_->anim.unsaved_clips[params_.clip_name] = true;
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_ADD_KEYFRAME_OPERATION_INL_H
