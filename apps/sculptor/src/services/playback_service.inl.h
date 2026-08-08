#pragma once

#ifndef VW_SCULPTOR_PLAYBACK_SERVICE_INL_H
#define VW_SCULPTOR_PLAYBACK_SERVICE_INL_H

namespace vw::sculptor {

inline playback_service::playback_service(
    engine_type& eng, app_state& state
)
    : engine_(&eng), state_(&state) {}

inline void playback_service::toggle_playback() const {
    if (state_->anim.selected_clip_name.empty() || state_->scene.root_name.empty() ||
        !state_->scene.name_to_entity.contains(state_->scene.root_name)) {
        return;
    }

    auto& world          = engine_->get_world();
    const auto root_ent  = state_->scene.name_to_entity[state_->scene.root_name];
    const auto layer_idx = state_->anim.get_layer_for_clip(state_->anim.selected_clip_name);

    if (!world.has<gfx::animation_player_component>(root_ent)) {
        world.modify(root_ent).with<gfx::animation_player_component>();
    }

    auto& anim_sys     = world.system<gfx::animation_system>();
    const auto& player = world.get<gfx::animation_player_component>(root_ent);

    const bool is_same_clip = player.has_layer(layer_idx) && player.get_layer(layer_idx).clip &&
        player.get_layer(layer_idx).clip->get_name() == state_->anim.selected_clip_name;

    if (is_same_clip && player.get_layer(layer_idx).state == asset::animation_state::playing) {
        anim_sys.modify_player(root_ent).layer(layer_idx).pause();
        return;
    }

    const auto& clip_reg = world.resource<asset::animation_clip_registry>();
    const auto clip      = clip_reg.get(state_->anim.selected_clip_name);
    if (!clip) {
        return;
    }

    if (is_same_clip && player.get_layer(layer_idx).state == asset::animation_state::paused) {
        anim_sys.modify_player(root_ent).layer(layer_idx).resume();
    } else {
        const auto& cs      = state_->anim.get_clip_settings(state_->anim.selected_clip_name);
        const auto& bt      = cs.blend_transition;
        const auto modifier = anim_sys.modify_player(root_ent).layer(layer_idx);

        modifier.blend_to(clip, bt.duration > 0.f ? std::optional{bt} : std::nullopt);
        modifier.set_playback_speed(cs.playback_speed);
        modifier.set_loop_mode(cs.loop_mode);
        modifier.set_fade_in(cs.fade_in);
        modifier.set_fade_out(cs.fade_out);
        if (cs.fade_in.duration > 0.f) {
            modifier.play(cs.fade_in);
        } else {
            modifier.play();
        }
    }
}

inline void playback_service::stop_playback() const {
    if (!state_->scene.root_name.empty() &&
        state_->scene.name_to_entity.contains(state_->scene.root_name)) {
        auto& world          = engine_->get_world();
        const auto root_ent  = state_->scene.name_to_entity[state_->scene.root_name];
        const auto layer_idx = state_->anim.get_layer_for_clip(state_->anim.selected_clip_name);
        if (world.has<gfx::animation_player_component>(root_ent)) {
            auto& anim_sys      = world.system<gfx::animation_system>();
            const auto& cs      = state_->anim.get_clip_settings(state_->anim.selected_clip_name);
            const auto modifier = anim_sys.modify_player(root_ent).layer(layer_idx);
            if (cs.fade_out.duration > 0.f) {
                modifier.stop(cs.fade_out);
            } else {
                modifier.stop();
            }
        }
    }
    state_->anim.timeline_cursor = 0.f;
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_PLAYBACK_SERVICE_INL_H
