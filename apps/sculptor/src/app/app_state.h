#pragma once

#ifndef VW_SCULPTOR_STATE_H
#define VW_SCULPTOR_STATE_H

namespace vw::sculptor {

struct ui_state {
    float left_size_voffset  = 0.f;
    float right_side_voffset = 0.f;
};

struct app_state {
    ui_state ui;

    color selected_color = colors::white;
    std::string selected_name;
    std::string root_name;
    std::unordered_map<std::string, gfx::entity> name_to_entity;
    std::unordered_map<gfx::entity, std::string> entity_to_name;
};

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_STATE_H
