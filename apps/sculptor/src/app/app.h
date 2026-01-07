#pragma once

#ifndef VW_SCULPTOR_APP_H
#define VW_SCULPTOR_APP_H

#include <vw/gfx.h>

#include "state.h"
#include "tools/entity_creation_tool.h"
#include "ui/color_palette_panel.h"
#include "ui/entity_properties_panel.h"
#include "ui/entity_tree_panel.h"
#include "ui/tool_panel.h"

namespace vw::sculptor {

class app final : public gfx::app<> {
public:
    explicit app(gfx::engine<>& eng);

    void render(float delta_time) override;

private:
    void handle_key_press(gfx::keyboard::key key);
    void handle_mouse_press(const gfx::mouse_press_event& event);

    gfx::fps_camera_controller camera_controller_;
    bool camera_movement_enabled_ = false;

    state state_;

    entity_creation_tool<> entity_creation_tool_;

    tool_panel tool_panel_;
    color_palette_panel color_palette_panel_;
    entity_properties_panel<> entity_properties_panel_;
    entity_tree_panel<> entity_tree_panel_;
};

}  // namespace vw::sculptor

#include "app.inl.h"

#endif  // VW_SCULPTOR_APP_H
