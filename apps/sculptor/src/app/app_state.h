#pragma once

#ifndef VW_SCULPTOR_STATE_H
#define VW_SCULPTOR_STATE_H
#include "vw/gfx/world/entity_guard.h"

namespace vw::sculptor {

enum class tools : int {
    invalid       = 0,
    select_entity = 1,
    add_voxel     = 2,
    remove_voxel  = 3,
    paint_voxel   = 4,
};

struct ui_state {
    float left_size_voffset  = 0.f;
    float right_side_voffset = 0.f;
};

struct app_state {
    using entity_guard_type = gfx::entity_guard<>;

    ui_state ui;

    tools selected_tool  = tools::select_entity;
    color selected_color = colors::white;

    std::string selected_name;
    std::string root_name;
    std::unordered_map<std::string, gfx::entity> name_to_entity;
    std::unordered_map<gfx::entity, std::string> entity_to_name;

    std::vector<std::unique_ptr<entity_guard_type>> entities;
};

}  // namespace vw::sculptor

template <>
struct std::hash<vw::sculptor::tools> {
    auto operator()(
        vw::sculptor::tools t
    ) const noexcept -> size_t {
        return std::hash<vw::uint32>()(static_cast<vw::uint32>(t));
    }
};

#endif  // VW_SCULPTOR_STATE_H
