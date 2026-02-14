#pragma once

#ifndef VW_SCULPTOR_TIMELINE_PANEL_H
#define VW_SCULPTOR_TIMELINE_PANEL_H

#include <vw/gfx/engine/engine.h>

#include "app/app_state.h"
#include "create_keyframe_modal.h"
#include "delete_track_modal.h"
#include "operations/operation_manager.h"

namespace vw::sculptor {

class timeline_panel final {
public:
    using engine_type = gfx::engine<>;

    timeline_panel(engine_type& eng, app_state& st, operation_manager& op_manager);

    void render(float delta_time);

private:
    void render_toolbar(float clip_duration);
    void render_tracks();
    void render_track_row(
        const gfx::animation_track& track,
        const std::shared_ptr<gfx::animation_clip>& clip,
        float track_area_width,
        float clip_duration,
        float scroll_offset
    );
    void render_keyframe_markers(
        const gfx::animation_channel_variant& channel_var,
        const std::string& track_name,
        gfx::animation_property prop,
        float track_width,
        float clip_duration,
        float scroll_offset
    );
    void render_playhead(
        float track_area_x,
        float track_width,
        float clip_duration,
        float area_top,
        float area_bottom,
        float scroll_offset
    );

    void delete_selected_keyframe();

    void save_transforms();
    void restore_transforms();
    void apply_scrub(float time);

    engine_type* engine_;
    app_state* state_;
    operation_manager* op_manager_;

    create_keyframe_modal create_kf_modal_;
    delete_track_modal delete_track_modal_;

    float playback_speed_ = 1.0f;
    int loop_mode_index_  = 0;
    float zoom_percent_   = 100.f;
    float scroll_offset_  = 0.f;

    bool scrollbar_dragging_      = false;
    float scrollbar_drag_start_   = 0.f;
    float scrollbar_scroll_start_ = 0.f;

    float prev_cursor_time_  = -1.f;
    bool keyframe_clicked_   = false;
};

}  // namespace vw::sculptor

#include "timeline_panel.inl.h"

#endif  // VW_SCULPTOR_TIMELINE_PANEL_H
