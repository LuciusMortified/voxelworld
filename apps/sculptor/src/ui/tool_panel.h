#pragma once

#ifndef VW_SCULPTOR_TOOL_PANEL_H
#define VW_SCULPTOR_TOOL_PANEL_H

#include <app/state.h>

namespace vw::sculptor {

class tool_panel {
public:
    explicit tool_panel(state& st);

    void render(float delta_time);

private:
    state* state_;
};

}  // namespace vw::sculptor

#include "tool_panel.inl.h"

#endif  // VW_SCULPTOR_TOOL_PANEL_H
