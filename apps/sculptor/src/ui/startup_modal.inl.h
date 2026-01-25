#pragma once

#ifndef VW_SCULPTOR_STARTUP_MODAL_INL_H
#define VW_SCULPTOR_STARTUP_MODAL_INL_H

namespace vw::sculptor {

inline startup_modal::startup_modal(
    engine_type& eng, app_state& state
)
    : engine_(&eng), state_(&state) {}

inline void startup_modal::render(
    float /*delta_time*/
) {
    if (state_->ui.need_startup_modal) {
        ImGui::OpenPopup("Welcome to Sculptor");
        state_->ui.need_startup_modal = false;
    }

    ImGuiWindowFlags dialog_flags =          //
        ImGuiWindowFlags_AlwaysAutoResize |  //
        ImGuiWindowFlags_NoMove;
    if (ImGui::BeginPopupModal("Welcome to Sculptor", nullptr, dialog_flags)) {
        ImGui::Text("Select next step to get started:");
        ImGui::Spacing();
        if (ImGui::Button("New File")) {
            state_->ui.need_new_file_modal = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Open File")) {
            state_->ui.need_open_file_modal = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_STARTUP_MODAL_INL_H
