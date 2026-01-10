#pragma once

#ifndef VW_SCULPTOR_MENU_BAR_INL_H
#define VW_SCULPTOR_MENU_BAR_INL_H

namespace vw::sculptor {

template <typename WC>
menu_bar<WC>::menu_bar(
    engine_type& eng, app_state& state, operation_manager& op_manager
)
    : engine_(&eng), state_(&state), op_manager_(&op_manager) {}

template <typename WC>
void menu_bar<WC>::render(
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
        if (ImGui::MenuItem("New", "Ctrl+N")) {
            // TODO: new project
        }
        if (ImGui::MenuItem("Open", "Ctrl+O")) {
            // TODO: open project
        }
        if (ImGui::MenuItem("Save", "Ctrl+S")) {
            // TODO: save project
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Exit", "Alt+F4")) {
            // TODO: exit
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

    if (ImGui::BeginMenu("Help")) {
        if (ImGui::MenuItem("About")) {
            ImGui::ShowAboutWindow();
        }

        ImGui::EndMenu();
    }

    state_->ui.left_size_voffset += 20.0f;
    state_->ui.right_side_voffset += 20.0f;

    ImGui::EndMenuBar();

    ImGui::End();
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_MENU_BAR_INL_H
