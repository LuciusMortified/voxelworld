#pragma once

#ifndef VW_SCULPTOR_TOOL_PANEL_H
#define VW_SCULPTOR_TOOL_PANEL_H

#include "app/app_state.h"
#include "ui_utils.h"

namespace vw::sculptor {

class tool_panel {
public:
    explicit tool_panel(app_state& st);

    void render(float delta_time);

private:
    app_state* state_;

    void render_tool_button(tools tool, std::string_view label, std::string_view shortcut);
};

}  // namespace vw::sculptor

#include "tool_panel.inl.h"

#endif  // VW_SCULPTOR_TOOL_PANEL_H
