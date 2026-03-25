#pragma once

#ifndef VW_SCULPTOR_COLOR_PALETTE_PANEL_H
#define VW_SCULPTOR_COLOR_PALETTE_PANEL_H

#include "app/app_state.h"
#include "ui_utils.h"

namespace vw::sculptor {

class color_palette_panel final {
public:
    color_palette_panel(app_state& st, const block_registry& registry);

    void render(float delta_time);

private:
    static auto to_imvec4(color clr) -> ImVec4;

    app_state* state_;
    const block_registry* registry_;
};

}  // namespace vw::sculptor

#include "color_palette_panel.inl.h"

#endif  // VW_SCULPTOR_COLOR_PALETTE_PANEL_H
