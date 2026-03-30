#pragma once

#ifndef VW_SCULPTOR_ADD_TRACK_OPERATION_INL_H
#define VW_SCULPTOR_ADD_TRACK_OPERATION_INL_H

namespace vw::sculptor {

inline add_track_operation::add_track_operation(
    engine_type& engine, app_state& state, const add_track_params& params
)
    : base_operation(), engine_(&engine), state_(&state), params_(params) {}

inline void add_track_operation::execute() {
    const auto& registry = engine_->get_world().get_animation_clip_registry();
    const auto clip      = registry.get(params_.clip_name);
    if (!clip) {
        return;
    }

    gfx::animation_track track(params_.track_name);

    if (params_.property.has_value() && params_.keyframe.has_value()) {
        const auto prop = params_.property.value();
        const auto& kf  = params_.keyframe.value();

        if (prop == gfx::animation_property::rotation) {
            auto channel = gfx::make_animation_channel<gfx::animation_property::rotation>();
            channel.add(std::get<gfx::keyframe_quat>(kf));
            track.add<gfx::animation_property::rotation>(std::move(channel));
        } else if (prop == gfx::animation_property::position) {
            auto channel = gfx::make_animation_channel<gfx::animation_property::position>();
            channel.add(std::get<gfx::keyframe_vec3f>(kf));
            track.add<gfx::animation_property::position>(std::move(channel));
        } else if (prop == gfx::animation_property::scale) {
            auto channel = gfx::make_animation_channel<gfx::animation_property::scale>();
            channel.add(std::get<gfx::keyframe_vec3f>(kf));
            track.add<gfx::animation_property::scale>(std::move(channel));
        } else if (prop == gfx::animation_property::origin) {
            auto channel = gfx::make_animation_channel<gfx::animation_property::origin>();
            channel.add(std::get<gfx::keyframe_vec3f>(kf));
            track.add<gfx::animation_property::origin>(std::move(channel));
        }
    }

    clip->add_track(std::move(track));

    if (state_->scene.name_to_entity.contains(params_.track_name)) {
        const auto ent = state_->scene.name_to_entity[params_.track_name];
        auto* guard    = state_->scene.find_guard(ent);
        if (guard && !guard->has<gfx::animation_target_component>()) {
            added_target_component_ = true;
            guard->with<gfx::animation_target_component>();
            engine_->get_world().get_animation_system().modify_target(ent).set_target_name(
                params_.track_name
            );
        }
    }

    state_->anim.need_apply_pose                  = true;
    state_->anim.unsaved_clips[params_.clip_name] = true;
}

inline void add_track_operation::undo() {
    const auto& registry = engine_->get_world().get_animation_clip_registry();
    const auto clip      = registry.get(params_.clip_name);
    if (!clip) {
        return;
    }

    clip->remove_track(params_.track_name);
    state_->anim.need_apply_pose                  = true;
    state_->anim.unsaved_clips[params_.clip_name] = true;
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_ADD_TRACK_OPERATION_INL_H
