#pragma once

#ifndef VW_SCULPTOR_APP_H
#define VW_SCULPTOR_APP_H

#include <vw/gfx.h>

#include "app_state.h"
#include "operations/operation_manager.h"
#include "tools/base_tool.h"
#include "ui/color_palette_panel.h"
#include "ui/entity_properties_panel.h"
#include "ui/entity_tree_panel.h"
#include "ui/menu_bar.h"
#include "ui/tool_panel.h"

namespace vw::sculptor {

class app final : public gfx::app<> {
public:
    using engine_type = gfx::engine<>;

    explicit app(engine_type& eng);

    void render(float delta_time) override;

private:
    void handle_key_press(const gfx::key_press_event& ev);
    void handle_mouse_move(const gfx::mouse_move_event& ev);
    void handle_mouse_press(const gfx::mouse_press_event& ev);
    void handle_mouse_release(const gfx::mouse_release_event& ev);

    gfx::fps_camera_controller camera_controller_;
    bool camera_movement_enabled_ = false;

    app_state state_;
    operation_manager op_manager_;

    std::unordered_map<tools, std::unique_ptr<base_tool>> tools_;

    menu_bar menu_bar_;
    tool_panel tool_panel_;
    color_palette_panel color_palette_panel_;
    entity_properties_panel entity_properties_panel_;
    entity_tree_panel entity_tree_panel_;
};

}  // namespace vw::sculptor

#include "app.inl.h"

#endif  // VW_SCULPTOR_APP_H
