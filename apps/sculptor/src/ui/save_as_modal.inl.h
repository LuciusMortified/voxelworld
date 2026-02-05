#pragma once

#ifndef VW_SCULPTOR_SAVE_AS_MODAL_INL_H
#define VW_SCULPTOR_SAVE_AS_MODAL_INL_H

#include <filesystem>

#include "ui_utils.h"
#include "vw/gfx/world/serializers/vox_serializer.h"

namespace vw::sculptor {

inline save_as_modal::save_as_modal(
    engine_type& eng, app_state& st
)
    : engine_(&eng), state_(&st) {}

inline void save_as_modal::render(
    float /*delta_time*/
) {
    if (state_->ui.need_save_as_modal) {
        ImGui::OpenPopup("Save As");
        state_->ui.need_save_as_modal = false;

        // Pre-fill with current filename (without extension)
        namespace fs = std::filesystem;
        fs::path current_path(state_->filename);
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

        ImGui::EndPopup();
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

    // Check if we have a valid model to save
    if (state_->root_name.empty() || !state_->name_to_entity.contains(state_->root_name)) {
        error_ = "No model to save.";
        return false;
    }

    gfx::vox_serializer serializer{
        engine_->get_world(),
        state_->name_to_entity.at(state_->root_name),
        state_->entity_to_name
    };

    if (!serializer.serialize(filepath)) {
        error_ = "Failed to save file.";
        return false;
    }

    // Update state with new filename
    state_->filename            = filepath.filename().string();
    state_->has_unsaved_changes = false;

    auto title = std::format("Sculptor {} | {}", version_string, state_->filename);
    engine_->get_window().set_title(title);

    return true;
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_SAVE_AS_MODAL_INL_H
