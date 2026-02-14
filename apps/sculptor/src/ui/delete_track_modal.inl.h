#pragma once

#ifndef VW_SCULPTOR_DELETE_TRACK_MODAL_INL_H
#define VW_SCULPTOR_DELETE_TRACK_MODAL_INL_H

#include "operations/remove_track_operation.h"

namespace vw::sculptor {

inline delete_track_modal::delete_track_modal(
    engine_type& eng, app_state& st, operation_manager& op_manager
)
    : engine_(&eng), state_(&st), op_manager_(&op_manager) {}

inline void delete_track_modal::open(
    const std::string& track_name
) {
    need_open_  = true;
    track_name_ = track_name;
}

inline void delete_track_modal::render(
    float /*delta_time*/
) {
    if (need_open_) {
        ImGui::OpenPopup("Delete Track?");
        need_open_ = false;
    }

    ImGuiWindowFlags dialog_flags =          //
        ImGuiWindowFlags_AlwaysAutoResize |  //
        ImGuiWindowFlags_NoMove;
    if (ImGui::BeginPopupModal("Delete Track?", nullptr, dialog_flags)) {
        ImGui::Text("Delete track '%s' and all its keyframes?", track_name_.c_str());
        ImGui::Spacing();

        if (ImGui::Button("Delete")) {
            remove_track_params params = {
                .clip_name  = state_->selected_clip_name,
                .track_name = track_name_,
            };
            auto op = std::make_unique<remove_track_operation>(*engine_, *state_, params);
            op_manager_->execute(std::move(op));
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_DELETE_TRACK_MODAL_INL_H
