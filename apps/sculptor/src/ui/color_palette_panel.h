#pragma once

#ifndef VW_SCULPTOR_COLOR_PALETTE_PANEL_H
#define VW_SCULPTOR_COLOR_PALETTE_PANEL_H

#include "app/app_state.h"
#include "ui_utils.h"

namespace vw::sculptor {

class color_palette_panel {
public:
    explicit color_palette_panel(app_state& st);

    void render(float delta_time);

private:
    ImVec4 to_imvec4(const color& clr) const;

    app_state* state_;
};

}  // namespace vw::sculptor

#include "color_palette_panel.inl.h"

#endif  // VW_SCULPTOR_COLOR_PALETTE_PANEL_H
