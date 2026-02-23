#pragma once

#ifndef VW_SCULPTOR_STATE_INL_H
#define VW_SCULPTOR_STATE_INL_H

namespace vw::sculptor {

inline auto app_state::has_unsaved_clip(
    const std::string& name
) const -> bool {
    const auto it = unsaved_clips.find(name);
    return it != unsaved_clips.end() && it->second;
}

inline auto app_state::has_any_unsaved_clip() const -> bool {
    return std::ranges::any_of(unsaved_clips | std::views::values, [](bool v) { return v; });
}

inline auto app_state::get_layer_for_clip(
    const std::string& name
) const -> size_t {
    const auto it = clip_to_layer.find(name);
    return it != clip_to_layer.end() ? it->second : 0;
}

inline auto app_state::get_clip_settings(
    const std::string& name
) const -> clip_settings {
    const auto it = clip_settings_map.find(name);
    return it != clip_settings_map.end() ? it->second : clip_settings{};
}

inline auto app_state::get_clip_settings_mut(
    const std::string& name
) -> clip_settings& {
    return clip_settings_map[name];
}

inline auto app_state::find_guard(
    gfx::entity ent
) const -> entity_guard_type* {
    for (auto& g : entities) {
        if (g->get_entity() == ent)
            return g.get();
    }
    return nullptr;
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_STATE_INL_H
