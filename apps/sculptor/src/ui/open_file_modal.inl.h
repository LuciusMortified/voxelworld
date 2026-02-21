#pragma once

#ifndef VW_SCULPTOR_OPEN_FILE_MODAL_INL_H
#define VW_SCULPTOR_OPEN_FILE_MODAL_INL_H
#include "vw/gfx/world/serializers/vox_deserializer.h"

namespace vw::sculptor {

inline open_file_modal::open_file_modal(
    engine_type& eng, app_state& st
)
    : engine_(&eng), state_(&st) {}

inline void open_file_modal::render(
    float /*delta_time*/
) {
    if (state_->ui.need_open_file_modal) {
        ImGui::OpenPopup("Open File");
        state_->ui.need_open_file_modal = false;

        filename_.clear();
        load_existing_filenames_();
    }

    constexpr ImGuiWindowFlags dialog_flags =  //
        ImGuiWindowFlags_AlwaysAutoResize |    //
        ImGuiWindowFlags_NoMove;
    if (ImGui::BeginPopupModal("Open File", nullptr, dialog_flags)) {
        ImGui::Text("Existing Files:");
        ImGui::Spacing();

        // Список существующих файлов с фиксированной высотой
        const float list_height = ImGui::GetTextLineHeightWithSpacing() * 7.5f;

        constexpr ImGuiChildFlags child_flags =       //
            ImGuiChildFlags_AlwaysUseWindowPadding |  //
            ImGuiChildFlags_Borders;
        if (ImGui::BeginChild("##file_list", ImVec2(400.f, list_height), child_flags)) {
            for (const auto& f : existing_filenames_) {
                bool is_selected = filename_ == f;
                if (ImGui::Selectable(f.c_str(), is_selected)) {
                    filename_ = f;
                }
            }
            ImGui::EndChild();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (filename_.empty()) {
            ImGui::BeginDisabled();
        }
        if (ImGui::Button("Open")) {
            if (open_file_()) {
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

        ImGui::EndPopup();
    }
}

inline void open_file_modal::load_existing_filenames_() {
    existing_filenames_.clear();

    namespace fs = std::filesystem;
    const fs::path asset_dir_path{app_state::asset_dir_name};
    if (!fs::exists(asset_dir_path)) {
        log::critical("Asset directory does not exist: {}", asset_dir_path.string());
    }
    for (const auto& entry : fs::directory_iterator(asset_dir_path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".vox") {
            existing_filenames_.emplace_back(entry.path().filename().string());
        }
    }
}

inline auto open_file_modal::open_file_() -> bool {
    namespace fs = std::filesystem;

    gfx::vox_deserializer deserializer{engine_->get_world()};

    const fs::path filepath =  //
        fs::path{app_state::asset_dir_name} / fs::path{filename_};
    auto result = deserializer.deserialize(filepath);
    if (!result.has_value()) {
        error_ = std::format("Failed to open file: {}", filepath.string());
        return false;
    }

    *state_ = app_state{};

    state_->ui.need_startup_modal = false;

    state_->filename       = filename_;
    state_->root_name      = result->root_name;
    state_->selected_name  = result->root_name;
    state_->name_to_entity = std::move(result->name_to_entity);
    state_->entity_to_name = std::move(result->entity_to_name);
    state_->entities       = std::move(result->entities);

    const auto title = std::format("Sculptor {} | {}", version_string, state_->filename);
    engine_->get_window().set_title(title);

    return true;
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_OPEN_FILE_MODAL_INL_H
