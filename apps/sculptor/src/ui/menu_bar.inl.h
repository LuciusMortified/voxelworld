#pragma once

#ifndef VW_SCULPTOR_MENU_BAR_INL_H
#define VW_SCULPTOR_MENU_BAR_INL_H
#include "vw/gfx/world/serializers/vox_serializer.h"

namespace vw::sculptor {

inline menu_bar::menu_bar(
    engine_type& eng, app_state& state, operation_manager& op_manager
)
    : engine_(&eng), state_(&state), op_manager_(&op_manager) {}

inline void menu_bar::render(
    float delta_time
) {
    ImGuiWindowFlags menu_window_flags =          //
        ImGuiWindowFlags_MenuBar |                //
        ImGuiWindowFlags_NoTitleBar |             //
        ImGuiWindowFlags_NoCollapse |             //
        ImGuiWindowFlags_NoResize |               //
        ImGuiWindowFlags_NoMove |                 //
        ImGuiWindowFlags_NoBringToFrontOnFocus |  //
        ImGuiWindowFlags_NoNavFocus |             //
        ImGuiWindowFlags_NoBackground;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(ImVec2{viewport->WorkSize.x, 10.f});

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("MenuWindow", nullptr, menu_window_flags);
    ImGui::PopStyleVar(3);

    ImGui::BeginMenuBar();

    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New File", "Ctrl+N")) {
            state_->ui.need_new_file_modal = true;
        }
        if (ImGui::MenuItem("Open File", "Ctrl+O")) {
            state_->ui.need_open_file_modal = true;
        }
        if (ImGui::MenuItem("Save File", "Ctrl+S")) {
            gfx::vox_serializer serializer{
                engine_->get_world(),
                state_->name_to_entity.at(state_->root_name),
                state_->entity_to_name
            };

            namespace fs = std::filesystem;
            fs::path assets_dir_path{app_state::asset_dir_name};
            fs::path filepath{assets_dir_path / state_->filename};
            serializer.serialize(filepath);
        }
        if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {
            state_->ui.need_save_as_modal = true;
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Exit", "Alt+F4")) {
            engine_->shutdown();
        }

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit")) {
        if (ImGui::MenuItem("Undo", "Ctrl+Z", false, !op_manager_->is_undo_empty())) {
            op_manager_->undo();
        }
        if (ImGui::MenuItem("Redo", "Ctrl+Shift+Z", false, !op_manager_->is_redo_empty())) {
            op_manager_->redo();
        }

        ImGui::EndMenu();
    }

#if 0
    if (ImGui::BeginMenu("Help")) {
        if (ImGui::MenuItem("About")) {
            ImGui::ShowAboutWindow();
        }

        ImGui::EndMenu();
    }
#endif

    state_->ui.left_size_voffset += 20.0f;
    state_->ui.right_side_voffset += 20.0f;

    ImGui::EndMenuBar();

    ImGui::End();
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_MENU_BAR_INL_H
