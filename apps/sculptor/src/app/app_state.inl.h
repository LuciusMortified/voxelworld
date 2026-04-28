#pragma once

#ifndef VW_SCULPTOR_STATE_INL_H
#define VW_SCULPTOR_STATE_INL_H

#include "vw/ecs/world.h"

namespace vw::sculptor {

inline void scene_state::clear_entities(
    world_type& world
) {
    for (auto ent : entities) {
        world.destroy(ent);
    }
    entities.clear();
    name_to_entity.clear();
    entity_to_name.clear();
}

inline void socket_state::socket_preview::destroy_entities(
    world_type& world
) {
    for (auto ent : entities) {
        world.destroy(ent);
    }
    entities.clear();
}

inline void socket_state::erase_preview(
    const std::string& key, world_type& world
) {
    const auto it = socket_previews.find(key);
    if (it == socket_previews.end()) {
        return;
    }
    it->second.destroy_entities(world);
    socket_previews.erase(it);
}

inline void socket_state::erase_previews_for(
    const std::string& entity_name, world_type& world
) {
    const auto prefix = entity_name + ":";
    std::erase_if(socket_previews, [&](auto& kv) {
        if (kv.first.starts_with(prefix)) {
            kv.second.destroy_entities(world);
            return true;
        }
        return false;
    });
}

inline void socket_state::clear_all(
    world_type& world
) {
    for (auto& preview : socket_previews | std::views::values) {
        preview.destroy_entities(world);
    }
    socket_previews.clear();
}

inline void app_state::reset(
    world_type& world
) {
    scene.clear_entities(world);
    sockets.clear_all(world);
    *this = app_state{};
}

inline auto animation_state::has_unsaved_clip(
    const std::string& name
) const -> bool {
    const auto it = unsaved_clips.find(name);
    return it != unsaved_clips.end() && it->second;
}

inline auto animation_state::has_any_unsaved_clip() const -> bool {
    return std::ranges::any_of(unsaved_clips | std::views::values, [](bool v) { return v; });
}

inline auto animation_state::get_layer_for_clip(
    const std::string& name
) const -> size_t {
    const auto it = clip_to_layer.find(name);
    return it != clip_to_layer.end() ? it->second : 0;
}

inline auto animation_state::get_clip_settings(
    const std::string& name
) const -> clip_settings {
    const auto it = clip_settings_map.find(name);
    return it != clip_settings_map.end() ? it->second : clip_settings{};
}

inline auto animation_state::get_clip_settings_mut(
    const std::string& name
) -> clip_settings& {
    return clip_settings_map[name];
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_STATE_INL_H
