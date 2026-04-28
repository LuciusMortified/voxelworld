#pragma once

#ifndef VW_SCULPTOR_TIMELINE_PANEL_H
#define VW_SCULPTOR_TIMELINE_PANEL_H

#include <optional>

#include <vw/gfx/engine/engine.h>

#include "app/app_state.h"
#include "create_keyframe_modal.h"
#include "delete_track_modal.h"
#include "operations/operation_manager.h"
#include "services/clip_service.h"
#include "services/keyframe_service.h"

namespace vw::sculptor {

class timeline_panel final {
public:
    using engine_type = gfx::engine<>;

    timeline_panel(engine_type& eng, app_state& st, operation_manager& op_manager,
                   clip_service& clip_svc, keyframe_service& kf_svc);

    void render(float delta_time);

private:
    void render_toolbar(float clip_duration);
    void render_tracks();
    void render_track_row(
        const asset::animation_track& track,
        float track_area_width,
        float clip_duration,
        float scroll_offset
    );
    void render_time_ruler(
        ImVec2 ruler_start,
        float ruler_width,
        float track_area_width,
        float clip_duration
    ) const;
    void render_scrollbar(float usable_track_width, float track_area_width, float max_scroll);
    void render_track_context_menu(const std::string& target);
    void render_expanded_channels(
        const asset::animation_track& track,
        const std::string& target,
        float track_area_width,
        float clip_duration,
        float scroll_offset
    );
    void render_keyframe_markers(
        const asset::animation_channel_variant& channel_var,
        const std::string& track_name,
        asset::animation_property prop,
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
    ) const;

    void render_playback_controls(const std::shared_ptr<asset::animation_clip>& clip);
    void render_clip_blend_controls_() const;
    void handle_play(ecs::entity root, const std::shared_ptr<asset::animation_clip>& clip) const;
    void handle_pause(ecs::entity root) const;
    void handle_stop(ecs::entity root) const;
    auto try_get_root_entity() const -> std::optional<ecs::entity>;

    [[nodiscard]] auto is_current_layer_playing() const -> bool;
    [[nodiscard]] auto is_clip_on_layer() const -> bool;
    void ensure_clip_on_layer(ecs::entity root) const;
    engine_type* engine_;
    app_state* state_;
    operation_manager* op_manager_;
    clip_service* clip_service_;
    keyframe_service* keyframe_service_;

    create_keyframe_modal create_kf_modal_;
    delete_track_modal delete_track_modal_;

    float zoom_percent_  = 100.f;
    float scroll_offset_ = 0.f;

    bool scrollbar_dragging_      = false;
    float scrollbar_drag_start_   = 0.f;
    float scrollbar_scroll_start_ = 0.f;

    float prev_cursor_time_     = -1.f;
    bool keyframe_clicked_      = false;
    std::string prev_clip_name_;
};

}  // namespace vw::sculptor

#include "timeline_panel.inl.h"

#endif  // VW_SCULPTOR_TIMELINE_PANEL_H
