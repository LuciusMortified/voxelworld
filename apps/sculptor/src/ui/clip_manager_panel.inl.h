#pragma once

#ifndef VW_SCULPTOR_CLIP_MANAGER_PANEL_INL_H
#define VW_SCULPTOR_CLIP_MANAGER_PANEL_INL_H

#include "operations/delete_clip_operation.h"

namespace vw::sculptor {

inline clip_manager_panel::clip_manager_panel(
    engine_type& eng, app_state& st, operation_manager& op_manager
)
    : engine_(&eng)
    , state_(&st)
    , op_manager_(&op_manager)
    , create_modal_(eng, st, op_manager) {}

inline void clip_manager_panel::render(
    float delta_time
) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 window_pos       = ImVec2(
        viewport->WorkPos.x + viewport->WorkSize.x - 10,
        viewport->WorkPos.y + state_->ui.right_top_voffset + 10
    );
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(1.0f, 0.0f));

    ImGuiWindowFlags window_flags =          //
        ImGuiWindowFlags_NoSavedSettings |   //
        ImGuiWindowFlags_AlwaysAutoResize |  //
        ImGuiWindowFlags_NoMove;

    ImGui::Begin("Animation Clips", nullptr, window_flags);

    if (state_->ui.need_create_clip_modal) {
        create_modal_.open();
        state_->ui.need_create_clip_modal = false;
    }

    if (ImGui::Button("New")) {
        create_modal_.open();
    }

    bool has_selected =
        !state_->selected_clip_name.empty() &&
        engine_->get_world().get_animation_clip_registry().has(state_->selected_clip_name);

    ImGui::SameLine();
    ImGui::BeginDisabled(!has_selected);
    if (ImGui::Button("Delete")) {
        delete_clip_params params = {.name = state_->selected_clip_name};
        auto op = std::make_unique<delete_clip_operation>(*engine_, *state_, params);
        op_manager_->execute(std::move(op));
    }
    ImGui::EndDisabled();

    auto& registry = engine_->get_world().get_animation_clip_registry();
    auto& clips    = registry.all();

    ImGui::Separator();

    for (const auto& [name, clip] : clips) {
        bool is_selected = (state_->selected_clip_name == name);
        if (ImGui::Selectable(name.c_str(), is_selected)) {
            state_->selected_clip_name = name;
            state_->ui.show_timeline   = true;
            state_->selected_track_name.clear();
            state_->selected_keyframe_time = -1.f;
        }

        if (is_selected && clip) {
            ImGui::SameLine();
            ImGui::TextDisabled("(%.1fs)", clip->get_duration());
        }
    }

    create_modal_.render(delta_time);

    ImGui::Dummy({200.0f, 0.0f});

    state_->ui.right_top_voffset += ImGui::GetWindowHeight() + 10.0f;

    ImGui::End();
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_CLIP_MANAGER_PANEL_INL_H
