#pragma once

#ifndef VW_SCULPTOR_CLIP_SERVICE_INL_H
#define VW_SCULPTOR_CLIP_SERVICE_INL_H

#include <filesystem>

#include "operations/close_clip_operation.h"
#include "vw/gfx/world/serializers/voxa_deserializer.h"
#include "vw/gfx/world/serializers/voxa_serializer.h"

namespace vw::sculptor {

inline clip_service::clip_service(
    engine_type& eng, app_state& state, operation_manager& op_manager
)
    : engine_(&eng), state_(&state), op_manager_(&op_manager) {}

inline auto clip_service::save_clip(
    const std::string& clip_name
) const -> bool {
    namespace fs = std::filesystem;

    const auto& registry = engine_->get_world().get_animation_clip_registry();
    const auto clip      = registry.get(clip_name);
    if (!clip) {
        return false;
    }

    const fs::path asset_dir_path{app_state::asset_dir_name};
    const fs::path filepath = asset_dir_path / std::format("{}.voxa", clip->get_name());
    gfx::voxa_serializer serializer(*clip);
    const auto result = serializer.serialize(filepath);
    if (result.has_value()) {
        state_->anim.unsaved_clips[clip_name] = false;
    }
    return result.has_value();
}

inline void clip_service::save_all_clips() const {
    namespace fs = std::filesystem;

    const auto& registry = engine_->get_world().get_animation_clip_registry();
    const fs::path asset_dir_path{app_state::asset_dir_name};

    for (const auto& [name, clip] : registry.all()) {
        if (!clip) {
            continue;
        }

        fs::path filepath = asset_dir_path / std::format("{}.voxa", name);
        gfx::voxa_serializer serializer(*clip);
        if (serializer.serialize(filepath).has_value()) {
            state_->anim.unsaved_clips[name] = false;
        }
    }
}

inline auto clip_service::load_clip(
    const std::string& filename
) const -> bool {
    namespace fs = std::filesystem;

    const fs::path filepath = fs::path{app_state::asset_dir_name} / fs::path{filename};
    gfx::voxa_deserializer deserializer;
    const auto result = deserializer.deserialize(filepath);
    if (!result) {
        return false;
    }

    const auto& clip = *result;
    auto& registry   = engine_->get_world().get_animation_clip_registry();
    registry.add(clip->get_name(), clip);

    state_->anim.selected_clip_name = clip->get_name();
    state_->ui.show_timeline        = true;
    state_->anim.selected_track_name.clear();
    state_->anim.selected_keyframe_id = gfx::invalid_keyframe_id;

    return true;
}

inline void clip_service::close_clip(
    const std::string& clip_name
) const {
    close_clip_params params = {.name = clip_name};
    auto op = std::make_unique<close_clip_operation>(*engine_, *state_, params);
    op_manager_->execute(std::move(op));
}

inline void clip_service::enter_animation_mode() {
    if (state_->anim.animation_mode) {
        return;
    }

    save_transforms();
    state_->anim.animation_mode = true;
}

inline void clip_service::exit_animation_mode() {
    stop_all_layers();
    restore_transforms();
    state_->anim.animation_mode  = false;
    state_->anim.timeline_cursor = 0.f;
}

inline void clip_service::force_exit_animation_mode() {
    if (state_->anim.animation_mode) {
        exit_animation_mode();
    }
    state_->ui.show_timeline = false;
}

inline void clip_service::save_transforms() {
    if (state_->anim.has_saved_transforms) {
        return;
    }

    auto& world = engine_->get_world();
    state_->anim.saved_transforms.clear();

    for (const auto& [name, ent] : state_->scene.name_to_entity) {
        if (world.has_component<gfx::transform_component>(ent)) {
            auto& tc                             = world.get_component<gfx::transform_component>(ent);
            state_->anim.saved_transforms[name] = tc.get_transform();
        }
    }

    state_->anim.has_saved_transforms = true;
}

inline void clip_service::restore_transforms() {
    if (!state_->anim.has_saved_transforms) {
        return;
    }

    auto& world            = engine_->get_world();
    auto& transform_system = world.get_transform_system();

    for (const auto& [name, t] : state_->anim.saved_transforms) {
        if (state_->scene.name_to_entity.contains(name)) {
            const auto ent = state_->scene.name_to_entity[name];
            if (world.has_component<gfx::transform_component>(ent)) {
                transform_system.modify(ent).set_transform(t);
            }
        }
    }

    state_->anim.saved_transforms.clear();
    state_->anim.has_saved_transforms = false;
}

inline void clip_service::reset_all() {
    stop_all_layers();

    if (state_->anim.has_saved_transforms) {
        auto& world            = engine_->get_world();
        auto& transform_system = world.get_transform_system();
        for (const auto& [name, t] : state_->anim.saved_transforms) {
            if (state_->scene.name_to_entity.contains(name)) {
                const auto ent = state_->scene.name_to_entity[name];
                if (world.has_component<gfx::transform_component>(ent)) {
                    transform_system.modify(ent).set_transform(t);
                }
            }
        }
    }

    state_->anim.timeline_cursor = 0.f;
}

inline void clip_service::stop_layer_for_clip(
    const std::string& clip_name
) {
    if (state_->scene.root_name.empty() ||
        !state_->scene.name_to_entity.contains(state_->scene.root_name)) {
        return;
    }

    auto root   = state_->scene.name_to_entity[state_->scene.root_name];
    auto& world = engine_->get_world();
    if (!world.has_component<gfx::animation_player_component>(root)) {
        return;
    }

    const auto layer_idx = state_->anim.get_layer_for_clip(clip_name);
    const auto& player   = world.get_component<gfx::animation_player_component>(root);
    if (player.has_layer(layer_idx) && player.get_layer(layer_idx).clip &&
        player.get_layer(layer_idx).clip->get_name() == clip_name) {
        world.get_animation_system().modify_player(root).layer(layer_idx).stop();
    }
}

inline void clip_service::stop_all_layers() {
    if (state_->scene.root_name.empty() ||
        !state_->scene.name_to_entity.contains(state_->scene.root_name)) {
        return;
    }

    const auto root = state_->scene.name_to_entity[state_->scene.root_name];
    auto& world     = engine_->get_world();
    if (!world.has_component<gfx::animation_player_component>(root)) {
        return;
    }

    const auto& player = world.get_component<gfx::animation_player_component>(root);
    auto& anim_sys     = world.get_animation_system();
    for (size_t i = 0; i < player.layer_count(); ++i) {
        if (player.has_layer(i)) {
            anim_sys.modify_player(root).layer(i).clear();
        }
    }
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_CLIP_SERVICE_INL_H
