#pragma once

#ifndef VW_SCULPTOR_TIMELINE_PANEL_INL_H
#define VW_SCULPTOR_TIMELINE_PANEL_INL_H

#include "operations/add_track_operation.h"
#include "operations/remove_keyframe_operation.h"

namespace vw::sculptor {

inline timeline_panel::timeline_panel(
    engine_type& eng, app_state& st, operation_manager& op_manager
)
    : engine_(&eng)
    , state_(&st)
    , op_manager_(&op_manager)
    , create_kf_modal_(eng, st, op_manager)
    , delete_track_modal_(eng, st, op_manager) {}

inline void timeline_panel::render(
    float delta_time
) {
    keyframe_clicked_ = false;

    auto& clip_registry = engine_->get_world().get_animation_clip_registry();
    auto clip           = clip_registry.get(state_->selected_clip_name);
    if (!clip) {
        if (state_->has_saved_transforms) {
            restore_transforms();
        }
        state_->ui.show_timeline = false;
        return;
    }

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 pos(
        viewport->WorkPos.x + 10.f,
        viewport->WorkPos.y + viewport->WorkSize.y  //
            - state_->ui.left_bottom_voffset        //
            - 10.f
    );
    ImVec2 size(viewport->WorkSize.x - 20.f, state_->ui.bottom_panel_height - 10.f);
    ImGui::SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.0f, 1.0f));
    ImGui::SetNextWindowSize(size, ImGuiCond_Always);

    ImGuiWindowFlags window_flags =         //
        ImGuiWindowFlags_NoCollapse |       //
        ImGuiWindowFlags_NoSavedSettings |  //
        ImGuiWindowFlags_NoMove |           //
        ImGuiWindowFlags_NoResize |         //
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    auto title      = std::format("Timeline - {}###Timeline", state_->selected_clip_name);
    bool still_open = state_->ui.show_timeline;
    ImGui::Begin(title.c_str(), &still_open, window_flags);

    if (!still_open && state_->ui.show_timeline) {
        state_->selected_track_name.clear();
        state_->selected_keyframe_time = -1.f;
        state_->ui.show_timeline       = false;
        if (state_->has_saved_transforms) {
            restore_transforms();
        }
    }

    if (state_->is_previewing) {
        auto& world = engine_->get_world();
        if (!state_->root_name.empty() && state_->name_to_entity.contains(state_->root_name)) {
            auto root_ent = state_->name_to_entity[state_->root_name];
            if (world.has_component<gfx::animation_player_component>(root_ent)) {
                auto& player = world.get_component<gfx::animation_player_component>(root_ent);
                if (player.is_playing()) {
                    state_->timeline_cursor = player.get_current_time();
                } else {
                    state_->is_previewing = false;
                }
            }
        }
    }

    float clip_duration = clip->get_duration();
    if (clip_duration <= 0.f) {
        clip_duration = 1.f;
    }

    if (state_->need_step_forward) {
        state_->need_step_forward = false;
        float step = clip_duration * 0.01f;
        state_->timeline_cursor = std::min(state_->timeline_cursor + step, clip_duration);
    }
    if (state_->need_step_backward) {
        state_->need_step_backward = false;
        float step = clip_duration * 0.01f;
        state_->timeline_cursor = std::max(state_->timeline_cursor - step, 0.f);
    }

    if (!state_->is_previewing && std::abs(state_->timeline_cursor - prev_cursor_time_) > 0.0001f) {
        apply_scrub(state_->timeline_cursor);
        prev_cursor_time_ = state_->timeline_cursor;
    }

    render_toolbar(clip_duration);
    ImGui::Separator();
    render_tracks();

    create_kf_modal_.render(delta_time);
    delete_track_modal_.render(delta_time);

    state_->ui.left_bottom_voffset += ImGui::GetWindowHeight() + 10.f;

    ImGui::End();
}

inline void timeline_panel::render_toolbar(
    float clip_duration
) {
    auto& registry = engine_->get_world().get_animation_clip_registry();
    auto clip      = registry.get(state_->selected_clip_name);
    if (!clip) {
        return;
    }

    bool can_add_track =
        !state_->selected_name.empty() && clip && !clip->has_track(state_->selected_name);
    if (can_add_track) {
        if (ImGui::Button("+ Track")) {
            add_track_params params = {
                .clip_name  = state_->selected_clip_name,
                .track_name = state_->selected_name,
            };
            auto op = std::make_unique<add_track_operation>(*engine_, *state_, params);
            op_manager_->execute(std::move(op));

            state_->selected_track_name = state_->selected_name;
            state_->expanded_tracks.insert(state_->selected_name);
        }
        ImGui::SameLine();
    }

    bool can_add_kf = !state_->selected_track_name.empty() && !state_->selected_clip_name.empty() &&
        clip->has_track(state_->selected_track_name);
    if (can_add_kf) {
        if (ImGui::Button("+ Keyframe")) {
            create_kf_modal_.open(state_->selected_track_name);
        }
        ImGui::SameLine();
    }

    if (can_add_track || can_add_kf) {
        ImGui::TextDisabled("|");
        ImGui::SameLine();
    }

    if (state_->is_previewing) {
        if (ImGui::Button("||")) {
            if (!state_->root_name.empty() && state_->name_to_entity.contains(state_->root_name)) {
                auto root_ent = state_->name_to_entity[state_->root_name];
                auto& world   = engine_->get_world();
                if (world.has_component<gfx::animation_player_component>(root_ent)) {
                    world.get_animation_system().modify_player(root_ent).pause();
                    state_->is_previewing = false;
                }
            }
        }
    } else {
        if (ImGui::Button(">")) {
            if (!state_->root_name.empty() && state_->name_to_entity.contains(state_->root_name)) {
                auto root_ent = state_->name_to_entity[state_->root_name];
                auto& world   = engine_->get_world();

                if (!world.has_component<gfx::animation_player_component>(root_ent)) {
                    if (auto* guard = state_->find_guard(root_ent)) {
                        guard->with<gfx::animation_player_component>();
                    }
                }

                auto& anim_sys = world.get_animation_system();
                auto& player = world.get_component<gfx::animation_player_component>(root_ent);

                if (player.is_paused()) {
                    anim_sys.modify_player(root_ent).resume();
                } else {
                    anim_sys.modify_player(root_ent).set_clip(clip);
                    anim_sys.modify_player(root_ent).set_playback_speed(playback_speed_);

                    auto loop = static_cast<gfx::animation_loop_mode>(loop_mode_index_);
                    anim_sys.modify_player(root_ent).set_loop_mode(loop);
                    anim_sys.modify_player(root_ent).play();
                }
                state_->is_previewing = true;
            }
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("[]")) {
        if (!state_->root_name.empty() && state_->name_to_entity.contains(state_->root_name)) {
            auto root_ent = state_->name_to_entity[state_->root_name];
            auto& world   = engine_->get_world();
            if (world.has_component<gfx::animation_player_component>(root_ent)) {
                world.get_animation_system().modify_player(root_ent).stop();
            }
        }
        state_->is_previewing   = false;
        state_->timeline_cursor = 0.f;
    }

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Speed");
    ImGui::SameLine();
    ImGui::PushItemWidth(60.f);
    ImGui::DragFloat("##Speed", &playback_speed_, 0.01f, 0.1f, 5.0f, "%.2f");
    ImGui::PopItemWidth();

    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Loop");
    ImGui::SameLine();
    ImGui::PushItemWidth(80.f);
    const char* loop_modes[] = {"Once", "Loop", "Ping-Pong"};
    ImGui::Combo("##Loop", &loop_mode_index_, loop_modes, 3);
    ImGui::PopItemWidth();

    ImGui::SameLine();
    ImGui::Text("%.2f / %.2fs", state_->timeline_cursor, clip_duration);

    ImGui::SameLine();
    float avail  = ImGui::GetContentRegionAvail().x;
    float zoom_w = 120.f;
    if (avail > zoom_w) {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + avail - zoom_w);
    }
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Zoom");
    ImGui::SameLine();
    ImGui::PushItemWidth(60.f);
    ImGui::DragFloat("##Zoom", &zoom_percent_, 1.f, 100.f, 1000.f, "%.0f%%");
    ImGui::PopItemWidth();
    zoom_percent_ = std::clamp(zoom_percent_, 100.f, 1000.f);
}

inline void timeline_panel::render_tracks() {
    auto& clip_registry = engine_->get_world().get_animation_clip_registry();
    auto clip           = clip_registry.get(state_->selected_clip_name);
    if (!clip) {
        return;
    }

    float clip_duration = clip->get_duration();
    if (clip_duration <= 0.f) {
        clip_duration = 1.f;
    }

    ImGui::BeginChild(
        "TracksArea", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar
    );

    constexpr float label_width = 150.f;
    constexpr float end_padding = 20.f;
    float available_width       = ImGui::GetContentRegionAvail().x;
    float base_track_width      = available_width - label_width;
    if (base_track_width < 100.f) {
        base_track_width = 100.f;
    }

    float usable_track_width = base_track_width - end_padding;
    float track_area_width   = usable_track_width * (zoom_percent_ / 100.f);

    float max_scroll = std::max(0.f, track_area_width - usable_track_width);
    scroll_offset_   = std::clamp(scroll_offset_, 0.f, max_scroll);

    if (ImGui::IsWindowHovered() && std::abs(ImGui::GetIO().MouseWheel) > 0.f &&
        ImGui::GetIO().KeyCtrl) {
        float old_zoom = zoom_percent_;
        zoom_percent_ += ImGui::GetIO().MouseWheel * 10.f;
        zoom_percent_ = std::clamp(zoom_percent_, 100.f, 1000.f);

        float new_track_width = usable_track_width * (zoom_percent_ / 100.f);
        float new_max_scroll  = std::max(0.f, new_track_width - usable_track_width);

        if (old_zoom > 100.f) {
            scroll_offset_ = scroll_offset_ * (zoom_percent_ / old_zoom);
        }
        scroll_offset_   = std::clamp(scroll_offset_, 0.f, new_max_scroll);
        track_area_width = new_track_width;
        max_scroll       = new_max_scroll;
    }

    if (ImGui::IsWindowHovered() && std::abs(ImGui::GetIO().MouseWheel) > 0.f &&
        !ImGui::GetIO().KeyCtrl && max_scroll > 0.f) {
        scroll_offset_ -= ImGui::GetIO().MouseWheel * 30.f;
        scroll_offset_ = std::clamp(scroll_offset_, 0.f, max_scroll);
    }

    ImGui::Columns(2, "timeline_columns", false);
    ImGui::SetColumnWidth(0, label_width);

    float time_ruler_y = ImGui::GetCursorScreenPos().y;

    ImGui::TextDisabled("Tracks");
    ImGui::NextColumn();

    ImVec2 ruler_start = ImGui::GetCursorScreenPos();
    float ruler_width  = usable_track_width;

    auto* draw_list = ImGui::GetWindowDrawList();

    float visible_start = scroll_offset_ / track_area_width * clip_duration;
    float visible_end   = (scroll_offset_ + usable_track_width) / track_area_width * clip_duration;
    float visible_range = visible_end - visible_start;

    constexpr float min_tick_spacing = 60.f;
    int tick_count = std::max(2, static_cast<int>(ruler_width / min_tick_spacing));

    float time_per_tick = visible_range / static_cast<float>(tick_count);

    int precision = 1;
    if (time_per_tick < 0.001f) {
        precision = 4;
    } else if (time_per_tick < 0.01f) {
        precision = 3;
    } else if (time_per_tick < 0.1f) {
        precision = 2;
    }

    auto fmt = std::format("{{:.{}f}}", precision);

    for (int i = 0; i <= tick_count; ++i) {
        float t    = static_cast<float>(i) / static_cast<float>(tick_count);
        float time = visible_start + t * visible_range;
        float x    = ruler_start.x + t * ruler_width;
        draw_list->AddLine(
            ImVec2(x, ruler_start.y), ImVec2(x, ruler_start.y + 12.f), IM_COL32(180, 180, 180, 255)
        );
        auto label = std::vformat(fmt, std::make_format_args(time));
        draw_list->AddText(
            ImVec2(x + 2, ruler_start.y), IM_COL32(180, 180, 180, 255), label.c_str()
        );
    }
    ImGui::Dummy(ImVec2(ruler_width, 16.f));
    ImGui::NextColumn();

    float tracks_start_y = ImGui::GetCursorScreenPos().y;

    draw_list->ChannelsSplit(2);
    draw_list->ChannelsSetCurrent(1);

    auto& tracks = clip->get_tracks();
    for (const auto& track : tracks) {
        render_track_row(track, clip, usable_track_width, clip_duration, scroll_offset_);
    }

    float tracks_end_y = ImGui::GetCursorScreenPos().y;
    float track_area_x = ruler_start.x;

    ImGui::Columns(1);

    draw_list->ChannelsSetCurrent(0);
    render_playhead(
        track_area_x, usable_track_width, clip_duration, time_ruler_y, tracks_end_y, scroll_offset_
    );
    draw_list->ChannelsMerge();

    if (max_scroll > 0.f) {
        constexpr float bar_height = 10.f;
        float bar_width            = ImGui::GetContentRegionAvail().x;
        float ratio                = usable_track_width / track_area_width;
        float thumb_w              = std::max(20.f, bar_width * ratio);
        float thumb_x              = (scroll_offset_ / max_scroll) * (bar_width - thumb_w);

        ImVec2 bar_pos = ImGui::GetCursorScreenPos();
        auto* dl       = ImGui::GetWindowDrawList();
        dl->AddRectFilled(
            bar_pos,
            ImVec2(bar_pos.x + bar_width, bar_pos.y + bar_height),
            IM_COL32(60, 60, 60, 255)
        );

        bool thumb_hovered = false;
        ImVec2 mouse       = ImGui::GetMousePos();
        if (mouse.x >= bar_pos.x + thumb_x && mouse.x <= bar_pos.x + thumb_x + thumb_w &&
            mouse.y >= bar_pos.y && mouse.y <= bar_pos.y + bar_height) {
            thumb_hovered = true;
        }

        ImU32 thumb_color = thumb_hovered || scrollbar_dragging_ ?
            IM_COL32(180, 180, 180, 255) :
            IM_COL32(120, 120, 120, 255);

        dl->AddRectFilled(
            ImVec2(bar_pos.x + thumb_x, bar_pos.y),
            ImVec2(bar_pos.x + thumb_x + thumb_w, bar_pos.y + bar_height),
            thumb_color,
            3.f
        );

        ImGui::InvisibleButton("##scrollbar", ImVec2(bar_width, bar_height));

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
            if (thumb_hovered) {
                scrollbar_dragging_     = true;
                scrollbar_drag_start_   = mouse.x;
                scrollbar_scroll_start_ = scroll_offset_;
            } else {
                float click_ratio = (mouse.x - bar_pos.x) / bar_width;
                scroll_offset_    = click_ratio * max_scroll;
                scroll_offset_    = std::clamp(scroll_offset_, 0.f, max_scroll);
            }
        }

        if (scrollbar_dragging_) {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                float delta        = mouse.x - scrollbar_drag_start_;
                float scroll_range = bar_width - thumb_w;
                if (scroll_range > 0.f) {
                    scroll_offset_ = scrollbar_scroll_start_ + (delta / scroll_range) * max_scroll;
                    scroll_offset_ = std::clamp(scroll_offset_, 0.f, max_scroll);
                }
            } else {
                scrollbar_dragging_ = false;
            }
        }
    } else {
        scrollbar_dragging_ = false;
    }

    ImGui::EndChild();
}

inline void timeline_panel::render_track_row(
    const gfx::animation_track& track,
    const std::shared_ptr<gfx::animation_clip>& clip,
    float track_area_width,
    float clip_duration,
    float scroll_offset
) {
    const auto& target     = track.get_target_name();
    bool is_expanded       = state_->expanded_tracks.contains(target);
    bool is_track_selected = (state_->selected_track_name == target);

    ImGuiTreeNodeFlags node_flags =       //
        ImGuiTreeNodeFlags_OpenOnArrow |  //
        ImGuiTreeNodeFlags_OpenOnDoubleClick;

    if (is_track_selected) {
        node_flags |= ImGuiTreeNodeFlags_Selected;
    }
    if (is_expanded) {
        node_flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }

    auto node_id = std::format("{}##track", target);
    bool opened  = ImGui::TreeNodeEx(node_id.c_str(), node_flags);

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        state_->selected_track_name = target;
    }

    auto ctx_id = std::format("TrackCtx_{}", target);
    if (ImGui::BeginPopupContextItem(ctx_id.c_str())) {
        state_->selected_track_name = target;
        if (ImGui::MenuItem("Add Keyframe...")) {
            create_kf_modal_.open(target);
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Delete Track...")) {
            delete_track_modal_.open(target);
        }
        ImGui::EndPopup();
    }

    if (opened != is_expanded) {
        if (opened) {
            state_->expanded_tracks.insert(target);
        } else {
            state_->expanded_tracks.erase(target);
        }
    }

    ImGui::NextColumn();

    if (!opened) {
        constexpr gfx::animation_property props[] = {
            gfx::animation_property::position,
            gfx::animation_property::rotation,
            gfx::animation_property::scale,
            gfx::animation_property::origin
        };

        for (auto prop : props) {
            if (auto* channel_var = track.get_channel(prop)) {
                render_keyframe_markers(
                    *channel_var, target, prop, track_area_width, clip_duration, scroll_offset
                );
            }
        }
        ImGui::Dummy(ImVec2(track_area_width, 16.f));
    } else {
        ImGui::Dummy(ImVec2(track_area_width, 2.f));
    }

    ImGui::NextColumn();

    if (opened) {
        const char* prop_names[] = {"Position", "Rotation", "Scale", "Origin"};

        constexpr gfx::animation_property props[] = {
            gfx::animation_property::position,
            gfx::animation_property::rotation,
            gfx::animation_property::scale,
            gfx::animation_property::origin
        };

        for (int i = 0; i < 4; ++i) {
            auto prop = props[i];
            bool has  = track.has_channel(prop);

            if (has) {
                ImGui::Text("  %s", prop_names[i]);
            } else {
                ImGui::TextDisabled("  %s", prop_names[i]);
            }

            auto sub_ctx_id = std::format("SubTrackCtx_{}_{}", target, i);
            if (ImGui::BeginPopupContextItem(sub_ctx_id.c_str())) {
                auto label = std::format("Add {} Keyframe...", prop_names[i]);
                if (ImGui::MenuItem(label.c_str())) {
                    state_->selected_property = prop;
                    create_kf_modal_.open(target);
                }
                ImGui::EndPopup();
            }
            ImGui::NextColumn();

            if (has) {
                if (auto* channel_var = track.get_channel(prop)) {
                    render_keyframe_markers(
                        *channel_var, target, prop, track_area_width, clip_duration, scroll_offset
                    );
                }
            }
            ImGui::Dummy(ImVec2(track_area_width, 16.f));
            ImGui::NextColumn();
        }

        ImGui::TreePop();
    }
}

inline void timeline_panel::render_keyframe_markers(
    const gfx::animation_channel_variant& channel_var,
    const std::string& track_name,
    gfx::animation_property prop,
    float track_width,
    float clip_duration,
    float scroll_offset
) {
    auto* draw_list   = ImGui::GetWindowDrawList();
    ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
    float row_height  = 16.f;

    float scale = track_width * (zoom_percent_ / 100.f);

    std::visit(
        [&](const auto& channel) {
            const auto& keyframes = channel.get_keyframes();
            for (const auto& kf : keyframes) {
                float pixel_pos = (kf.time / clip_duration) * scale - scroll_offset;

                if (pixel_pos < -10.f || pixel_pos > track_width + 10.f) {
                    continue;
                }

                float x = cursor_pos.x + pixel_pos;
                float y = cursor_pos.y + row_height * 0.5f;

                bool is_selected = (state_->selected_track_name == track_name) &&
                    (state_->selected_property == prop) &&
                    (std::abs(state_->selected_keyframe_time - kf.time) < 0.0001f);

                ImU32 color =
                    is_selected ? IM_COL32(255, 200, 50, 255) : IM_COL32(200, 200, 200, 255);

                constexpr float diamond_size = 5.f;
                draw_list->AddQuadFilled(
                    ImVec2(x, y - diamond_size),
                    ImVec2(x + diamond_size, y),
                    ImVec2(x, y + diamond_size),
                    ImVec2(x - diamond_size, y),
                    color
                );

                ImVec2 mouse = ImGui::GetMousePos();
                if (std::abs(mouse.x - x) < diamond_size + 2.f &&
                    std::abs(mouse.y - y) < diamond_size + 2.f) {
                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                        state_->selected_track_name    = track_name;
                        state_->selected_property      = prop;
                        state_->selected_keyframe_time = kf.time;
                        keyframe_clicked_              = true;
                    }

                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                        state_->selected_track_name    = track_name;
                        state_->selected_property      = prop;
                        state_->selected_keyframe_time = kf.time;
                        ImGui::OpenPopup("KeyframeContextMenu");
                    }
                }
            }
        },
        channel_var
    );

    if (ImGui::BeginPopup("KeyframeContextMenu")) {
        if (ImGui::MenuItem("Delete Keyframe")) {
            delete_selected_keyframe();
        }
        ImGui::EndPopup();
    }
}

inline void timeline_panel::render_playhead(
    float track_area_x,
    float track_width,
    float clip_duration,
    float area_top,
    float area_bottom,
    float scroll_offset
) {
    if (clip_duration <= 0.f) {
        return;
    }

    float scale     = track_width * (zoom_percent_ / 100.f);
    float pixel_pos = (state_->timeline_cursor / clip_duration) * scale - scroll_offset;
    float x         = track_area_x + pixel_pos;

    if (pixel_pos >= -2.f && pixel_pos <= track_width + 2.f) {
        auto* draw_list = ImGui::GetWindowDrawList();

        float zone_left  = track_area_x;
        float zone_right = std::min(x, track_area_x + track_width);
        if (zone_right > zone_left) {
            draw_list->AddRectFilled(
                ImVec2(zone_left, area_top),
                ImVec2(zone_right, area_bottom),
                IM_COL32(255, 80, 80, 30)
            );
        }

        draw_list->AddLine(
            ImVec2(x, area_top), ImVec2(x, area_bottom), IM_COL32(255, 80, 80, 255), 2.0f
        );

        constexpr float tri_size = 6.f;
        draw_list->AddTriangleFilled(
            ImVec2(x - tri_size, area_top),
            ImVec2(x + tri_size, area_top),
            ImVec2(x, area_top + tri_size),
            IM_COL32(255, 80, 80, 255)
        );
    }

    ImVec2 mouse = ImGui::GetMousePos();
    if (!keyframe_clicked_ && mouse.x >= track_area_x &&
        mouse.x <= track_area_x + track_width && mouse.y >= area_top &&
        mouse.y <= area_bottom) {
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) ||
            ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            if (!ImGui::IsAnyItemActive()) {
                float local_x           = mouse.x - track_area_x + scroll_offset;
                float new_time          = (local_x / scale) * clip_duration;
                state_->timeline_cursor = std::clamp(new_time, 0.f, clip_duration);
            }
        }
    }
}

inline void timeline_panel::delete_selected_keyframe() {
    auto& clip_reg = engine_->get_world().get_animation_clip_registry();
    auto clip = clip_reg.get(state_->selected_clip_name);
    if (!clip) {
        return;
    }

    auto* track = clip->get_track(state_->selected_track_name);
    if (!track) {
        return;
    }

    auto* ch = track->get_channel(state_->selected_property);
    if (!ch) {
        return;
    }

    std::visit(
        [&](const auto& channel) {
            for (const auto& kf : channel.get_keyframes()) {
                if (std::abs(kf.time - state_->selected_keyframe_time) < 0.0001f) {
                    remove_keyframe_params params;
                    params.clip_name  = state_->selected_clip_name;
                    params.track_name = state_->selected_track_name;
                    params.property   = state_->selected_property;
                    params.keyframe   = keyframe_value(kf);
                    auto op =
                        std::make_unique<remove_keyframe_operation>(*engine_, *state_, params);
                    op_manager_->execute(std::move(op));
                    return;
                }
            }
        },
        *ch
    );
}

inline void timeline_panel::save_transforms() {
    if (state_->has_saved_transforms) {
        return;
    }

    auto& world = engine_->get_world();
    state_->saved_transforms.clear();

    for (const auto& [name, ent] : state_->name_to_entity) {
        if (world.has_component<gfx::transform_component>(ent)) {
            auto& tc                       = world.get_component<gfx::transform_component>(ent);
            state_->saved_transforms[name] = tc.get_transform();
        }
    }

    state_->has_saved_transforms = true;
}

inline void timeline_panel::restore_transforms() {
    if (!state_->has_saved_transforms) {
        return;
    }

    auto& world            = engine_->get_world();
    auto& transform_system = world.get_transform_system();

    for (const auto& [name, t] : state_->saved_transforms) {
        if (state_->name_to_entity.contains(name)) {
            auto ent = state_->name_to_entity[name];
            if (world.has_component<gfx::transform_component>(ent)) {
                transform_system.modify(ent).set_transform(t);
            }
        }
    }

    state_->saved_transforms.clear();
    state_->has_saved_transforms = false;
}

inline void timeline_panel::apply_scrub(
    float time
) {
    auto& clip_registry = engine_->get_world().get_animation_clip_registry();
    auto clip           = clip_registry.get(state_->selected_clip_name);
    if (!clip) {
        return;
    }

    save_transforms();

    auto& world            = engine_->get_world();
    auto& transform_system = world.get_transform_system();
    auto& tracks           = clip->get_tracks();

    for (const auto& track : tracks) {
        const auto& target = track.get_target_name();
        if (!state_->name_to_entity.contains(target)) {
            continue;
        }

        auto ent = state_->name_to_entity[target];
        if (!world.has_component<gfx::transform_component>(ent)) {
            continue;
        }

        auto result = track.get_transform(time);
        if (result.has_value()) {
            transform_system.modify(ent).set_transform(result.value());
        }
    }
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_TIMELINE_PANEL_INL_H
