#pragma once

#ifndef VW_SCULPTOR_SAVE_AS_MODAL_INL_H
#define VW_SCULPTOR_SAVE_AS_MODAL_INL_H

#include <filesystem>

#include "ui_utils.h"

namespace vw::sculptor {

inline save_as_modal::save_as_modal(
    engine_type& eng, app_state& st, file_service& file_svc
)
    : engine_(&eng), state_(&st), file_service_(&file_svc) {}

inline void save_as_modal::render(
    float /*delta_time*/
) {
    if (state_->ui.need_save_as_modal) {
        ImGui::OpenPopup("Save As");
        state_->ui.need_save_as_modal = false;

        namespace fs = std::filesystem;
        fs::path current_path(state_->file.filename);
        filename_ = current_path.stem().string();

        error_.clear();
        need_overwrite_confirmation_ = false;
        has_overwrite_confirmation_  = false;
    }

    ImGuiWindowFlags dialog_flags =          //
        ImGuiWindowFlags_AlwaysAutoResize |  //
        ImGuiWindowFlags_NoMove;
    if (ImGui::BeginPopupModal("Save As", nullptr, dialog_flags)) {
        if (need_overwrite_confirmation_) {
            render_overwrite_confirmation();
        } else {
            render_save_form();
        }
        ImGui::EndPopup();
    }
}

inline void save_as_modal::render_overwrite_confirmation() {
    ImGui::TextColored(ImVec4{1.0f, 1.0f, 0.0f, 1.0f}, "File already exists. Overwrite?");
    ImGui::Spacing();

    if (ImGui::Button("Yes")) {
        need_overwrite_confirmation_ = false;
        has_overwrite_confirmation_  = true;
        if (save_file_()) {
            ImGui::CloseCurrentPopup();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("No")) {
        need_overwrite_confirmation_ = false;
    }
}

inline void save_as_modal::render_save_form() {
    if (!error_.empty()) {
        ImGui::TextColored(ImVec4{1.0f, 0.0f, 0.0f, 1.0f}, "%s", error_.c_str());
        ImGui::Spacing();
    }

    imgui_input_text_string("Filename", filename_);

    ImGui::Spacing();

    if (filename_.empty()) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Save")) {
        if (save_file_()) {
            ImGui::CloseCurrentPopup();
        }
    }
    if (filename_.empty()) {
        ImGui::EndDisabled();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
        ImGui::CloseCurrentPopup();
    }
}

inline auto save_as_modal::save_file_() -> bool {
    namespace fs = std::filesystem;

    fs::path asset_dir_path(app_state::asset_dir_name);
    fs::path filepath(asset_dir_path / filename_);
    if (filepath.extension() != ".vox") {
        filepath.replace_extension("vox");
    }

    if (!has_overwrite_confirmation_ && fs::exists(filepath)) {
        need_overwrite_confirmation_ = true;
        return false;
    }

    if (!file_service_->save_as(filepath)) {
        error_ = "Failed to save file.";
        return false;
    }

    return true;
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_SAVE_AS_MODAL_INL_H
