#pragma once

#ifndef VW_SCULPTOR_APP_H
#define VW_SCULPTOR_APP_H

#include <vw/gfx.h>

#include "app_state.h"
#include "operations/operation_manager.h"
#include "services/clip_service.h"
#include "services/file_service.h"
#include "services/keyframe_service.h"
#include "services/playback_service.h"
#include "tools/base_tool.h"
#include "ui/color_palette_panel.h"
#include "ui/entity_properties_panel.h"
#include "ui/entity_tree_panel.h"
#include "ui/menu_bar.h"
#include "ui/new_file_modal.h"
#include "ui/open_file_modal.h"
#include "ui/save_as_modal.h"
#include "ui/startup_modal.h"
#include "ui/clip_manager_panel.h"
#include "ui/keyframe_properties_panel.h"
#include "ui/timeline_panel.h"
#include "ui/socket_panel.h"
#include "ui/tool_panel.h"

namespace vw::sculptor {

class app final : public gfx::app {
public:
    using engine_type = gfx::engine;

    explicit app(engine_type& eng);
    ~app() override;

    void render(float delta_time) override;

private:
    void handle_key_press(const plat::key_press_event& ev);
    void handle_file_shortcuts(const plat::key_press_event& ev);
    void handle_mouse_move(const plat::mouse_move_event& ev);
    void handle_mouse_press(const plat::mouse_press_event& ev);
    void handle_mouse_release(const plat::mouse_release_event& ev);

    void handle_animation_actions_();
    void update_title_();
    static void init_asset_dir_();

    gfx::free_camera_controller camera_controller_;
    bool camera_movement_enabled_ = false;
    bool prev_unsaved_state_      = false;
    bool prev_clip_unsaved_state_ = false;
    bool prev_animation_mode_     = false;
    std::string prev_clip_name_;
    std::string prev_filename_;

    app_state state_;
    operation_manager op_manager_;
    file_service file_service_;
    clip_service clip_service_;
    playback_service playback_service_;
    keyframe_service keyframe_service_;

    tools active_tool_ = tools::add_voxel;
    std::unordered_map<tools, std::unique_ptr<base_tool>> tools_;

    menu_bar menu_bar_;
    tool_panel tool_panel_;
    color_palette_panel color_palette_panel_;
    entity_properties_panel entity_properties_panel_;
    socket_panel socket_panel_;
    keyframe_properties_panel keyframe_properties_panel_;
    entity_tree_panel entity_tree_panel_;
    clip_manager_panel clip_manager_panel_;
    timeline_panel timeline_panel_;

    startup_modal startup_modal_;
    new_file_modal new_file_modal_;
    open_file_modal open_file_modal_;
    save_as_modal save_as_modal_;
};

}  // namespace vw::sculptor

#include "app.inl.h"

#endif  // VW_SCULPTOR_APP_H
