#pragma once

#ifndef VW_SCULPTOR_NEW_FILE_MODAL_INL_H
#define VW_SCULPTOR_NEW_FILE_MODAL_INL_H

#include <filesystem>

#include "ui_utils.h"

namespace vw::sculptor {

inline new_file_modal::new_file_modal(
    engine_type& eng, app_state& st
)
    : engine_(&eng), state_(&st) {}

inline void new_file_modal::render(
    float /*delta_time*/
) {
    if (state_->ui.need_new_file_modal) {
        ImGui::OpenPopup("New File");
        state_->ui.need_new_file_modal = false;

        filename_.clear();
        error_.clear();
        need_overwrite_confirmation_ = false;
        has_overwrite_confirmation_  = false;
    }

    ImGuiWindowFlags dialog_flags =          //
        ImGuiWindowFlags_AlwaysAutoResize |  //
        ImGuiWindowFlags_NoMove;
    if (ImGui::BeginPopupModal("New File", nullptr, dialog_flags)) {
        if (need_overwrite_confirmation_) {
            ImGui::TextColored(ImVec4{1.0f, 1.0f, 0.0f, 1.0f}, "File already exists. Overwrite?");
            ImGui::Spacing();

            if (ImGui::Button("Yes")) {
                need_overwrite_confirmation_ = false;
                has_overwrite_confirmation_  = true;
                if (create_file_()) {
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("No")) {
                need_overwrite_confirmation_ = false;
            }
        } else {
            if (!error_.empty()) {
                ImGui::TextColored(ImVec4{1.0f, 0.0f, 0.0f, 1.0f}, "%s", error_.c_str());
                ImGui::Spacing();
            }

            imgui_input_text_string("Filename", filename_);

            ImGui::Spacing();

            if (filename_.empty()) {
                ImGui::BeginDisabled();
            }
            if (ImGui::Button("Create")) {
                if (create_file_()) {
                    ImGui::CloseCurrentPopup();
                }
            }
            if (filename_.empty()) {
                ImGui::EndDisabled();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                ImGui::CloseCurrentPopup();
                if (state_->filename.empty()) {
                    state_->ui.need_startup_modal = true;
                }
            }
        }

        ImGui::EndPopup();
    }
}

inline bool new_file_modal::create_file_() {
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

    std::ofstream file(filepath, std::ios::trunc);
    if (!file.is_open()) {
        error_ = "Failed to create file.";
        return false;
    }

    *state_ = app_state{};

    state_->ui.need_startup_modal = false;

    state_->filename = filepath.filename().string();

    auto title = std::format("Sculptor {} | {}", version_string, state_->filename);
    engine_->get_window().set_title(title);

    return true;
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_NEW_FILE_MODAL_INL_H
