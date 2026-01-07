#pragma once

#ifndef VW_SCULPTOR_COLOR_PALETTE_PANEL_H
#define VW_SCULPTOR_COLOR_PALETTE_PANEL_H

#include "app/state.h"

namespace vw::sculptor {

class color_palette_panel {
public:
    explicit color_palette_panel(state& st);

    void render(float delta_time);

private:
    ImVec4 to_imvec4(const color& clr) const;

    state* state_;
};

}  // namespace vw::sculptor

#include "color_palette_panel.inl.h"

#endif  // VW_SCULPTOR_COLOR_PALETTE_PANEL_H
