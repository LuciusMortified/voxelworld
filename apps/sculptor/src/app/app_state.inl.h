#pragma once

#ifndef VW_SCULPTOR_STATE_INL_H
#define VW_SCULPTOR_STATE_INL_H

namespace vw::sculptor {

inline auto app_state::has_unsaved_clip(
    const std::string& name
) const -> bool {
    auto it = unsaved_clips.find(name);
    return it != unsaved_clips.end() && it->second;
}

inline auto app_state::has_any_unsaved_clip() const -> bool {
    return std::ranges::any_of(unsaved_clips | std::views::values, [](bool v) { return v; });
}

inline auto app_state::find_guard(
    gfx::entity ent
) -> entity_guard_type* {
    for (auto& g : entities) {
        if (g->get_entity() == ent)
            return g.get();
    }
    return nullptr;
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_STATE_INL_H
