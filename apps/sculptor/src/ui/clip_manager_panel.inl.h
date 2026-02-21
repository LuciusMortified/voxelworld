#pragma once

#ifndef VW_SCULPTOR_CLIP_MANAGER_PANEL_INL_H
#define VW_SCULPTOR_CLIP_MANAGER_PANEL_INL_H

#include <filesystem>

#include "operations/close_clip_operation.h"
#include "vw/gfx/world/serializers/voxa_deserializer.h"
#include "vw/gfx/world/serializers/voxa_serializer.h"

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

    bool still_open = true;
    ImGui::Begin("Animation Clips", &still_open, window_flags);
    if (!still_open) {
        state_->ui.show_clip_manager = false;
    }

    if (state_->ui.need_create_clip_modal) {
        create_modal_.open();
        state_->ui.need_create_clip_modal = false;
    }

    if (state_->ui.need_save_clip) {
        state_->ui.need_save_clip = false;
        save_clip_();
    }

    if (state_->ui.need_load_clip_modal) {
        load_voxa_filenames_();
        selected_load_filename_.clear();
        ImGui::OpenPopup("Open Animation");
        state_->ui.need_load_clip_modal = false;
    }

    if (state_->ui.need_close_clip) {
        state_->ui.need_close_clip = false;
        if (state_->has_unsaved_clip(state_->selected_clip_name)) {
            need_close_confirm_popup_ = true;
        } else {
            close_clip_();
        }
    }

    bool has_selected =
        !state_->selected_clip_name.empty() &&
        engine_->get_world().get_animation_clip_registry().has(state_->selected_clip_name);

    if (ImGui::Button("New")) {
        create_modal_.open();
    }

    ImGui::SameLine();
    if (ImGui::Button("Open")) {
        load_voxa_filenames_();
        selected_load_filename_.clear();
        need_load_popup_ = true;
    }

    ImGui::SameLine();
    ImGui::BeginDisabled(!has_selected);
    if (ImGui::Button("Save")) {
        save_clip_();
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::BeginDisabled(!has_selected);
    if (ImGui::Button("Close")) {
        if (state_->has_unsaved_clip(state_->selected_clip_name)) {
            need_close_confirm_popup_ = true;
        } else {
            close_clip_();
        }
    }
    ImGui::EndDisabled();

    if (need_close_confirm_popup_) {
        ImGui::OpenPopup("Close Animation?");
        need_close_confirm_popup_ = false;
    }

    render_close_confirm_popup_();

    if (need_load_popup_) {
        ImGui::OpenPopup("Open Animation");
        need_load_popup_ = false;
    }
    render_load_popup_();

    auto& registry = engine_->get_world().get_animation_clip_registry();
    auto& clips    = registry.all();

    ImGui::Spacing();
    ImGui::Separator();

    for (const auto& [name, clip] : clips) {
        bool is_selected = (state_->selected_clip_name == name);
        if (ImGui::Selectable(name.c_str(), is_selected)) {
            if (!is_selected) {
                state_->selected_clip_name = name;
                state_->ui.show_timeline   = true;
                state_->selected_track_name.clear();
                state_->selected_keyframe_time = -1.f;
            }
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

inline void clip_manager_panel::render_close_confirm_popup_() {
    if (ImGui::BeginPopupModal("Close Animation?", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Animation \"%s\" has unsaved changes. Close anyway?",
                    state_->selected_clip_name.c_str());
        ImGui::Spacing();

        if (ImGui::Button("Save & Close")) {
            save_clip_();
            close_clip_();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Close")) {
            close_clip_();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

inline void clip_manager_panel::close_clip_() {
    close_clip_params params = {.name = state_->selected_clip_name};
    auto op = std::make_unique<close_clip_operation>(*engine_, *state_, params);
    op_manager_->execute(std::move(op));
}

inline void clip_manager_panel::render_load_popup_() {
    ImGuiWindowFlags dialog_flags =
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove;

    if (ImGui::BeginPopupModal("Open Animation", nullptr, dialog_flags)) {
        ImGui::Text("Animation files:");
        ImGui::Spacing();

        const float list_height     = ImGui::GetTextLineHeightWithSpacing() * 7.5f;
        ImGuiChildFlags child_flags =
            ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_Borders;
        if (ImGui::BeginChild("##voxa_file_list", ImVec2(400.f, list_height), child_flags)) {
            if (voxa_filenames_.empty()) {
                ImGui::TextDisabled("No .voxa files found");
            } else {
                for (const auto& filename : voxa_filenames_) {
                    bool is_selected = selected_load_filename_ == filename;
                    if (ImGui::Selectable(filename.c_str(), is_selected)) {
                        selected_load_filename_ = filename;
                    }
                }
            }
            ImGui::EndChild();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (selected_load_filename_.empty()) {
            ImGui::BeginDisabled();
        }
        if (ImGui::Button("Open")) {
            if (load_clip_(selected_load_filename_)) {
                ImGui::CloseCurrentPopup();
            }
        }
        if (selected_load_filename_.empty()) {
            ImGui::EndDisabled();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

inline void clip_manager_panel::load_voxa_filenames_() {
    namespace fs = std::filesystem;

    voxa_filenames_.clear();

    fs::path asset_dir_path{app_state::asset_dir_name};
    if (!fs::exists(asset_dir_path)) {
        return;
    }

    for (const auto& entry : fs::directory_iterator(asset_dir_path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".voxa") {
            voxa_filenames_.emplace_back(entry.path().filename().string());
        }
    }
}

inline auto clip_manager_panel::save_clip_() -> bool {
    namespace fs = std::filesystem;

    auto& registry = engine_->get_world().get_animation_clip_registry();
    auto clip = registry.get(state_->selected_clip_name);
    if (!clip) {
        return false;
    }

    fs::path asset_dir_path{app_state::asset_dir_name};
    fs::path filepath = asset_dir_path / std::format("{}.voxa", clip->get_name());

    gfx::voxa_serializer serializer(*clip);
    bool ok = serializer.serialize(filepath).has_value();
    if (ok) {
        state_->unsaved_clips[state_->selected_clip_name] = false;
    }
    return ok;
}

inline void clip_manager_panel::save_all_clips() {
    namespace fs = std::filesystem;

    auto& registry = engine_->get_world().get_animation_clip_registry();
    fs::path asset_dir_path{app_state::asset_dir_name};

    for (const auto& [name, clip] : registry.all()) {
        if (!clip) {
            continue;
        }

        fs::path filepath = asset_dir_path / std::format("{}.voxa", name);
        gfx::voxa_serializer serializer(*clip);
        if (serializer.serialize(filepath).has_value()) {
            state_->unsaved_clips[name] = false;
        }
    }
}

inline auto clip_manager_panel::load_clip_(const std::string& filename) -> bool {
    namespace fs = std::filesystem;

    fs::path filepath = fs::path{app_state::asset_dir_name} / fs::path{filename};

    gfx::voxa_deserializer deserializer;
    auto result = deserializer.deserialize(filepath);
    if (!result) {
        return false;
    }

    const auto& clip = *result;
    auto& registry = engine_->get_world().get_animation_clip_registry();
    registry.add(clip->get_name(), clip);

    state_->selected_clip_name = clip->get_name();
    state_->ui.show_timeline = true;
    state_->selected_track_name.clear();
    state_->selected_keyframe_time = -1.f;

    return true;
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_CLIP_MANAGER_PANEL_INL_H
