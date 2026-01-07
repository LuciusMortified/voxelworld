#pragma once

#ifndef VW_SCULPTOR_TOOL_PANEL_INL_H
#define VW_SCULPTOR_TOOL_PANEL_INL_H

namespace vw::sculptor {

inline tool_panel::tool_panel(
    state& st
)
    : state_(&st) {}

inline void tool_panel::render(
    float delta_time
) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 window_pos       = ImVec2(viewport->WorkPos.x + 10, viewport->WorkPos.y + 10);
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_FirstUseEver);

    ImGuiWindowFlags window_flags =         //
        ImGuiWindowFlags_NoCollapse |       //
        ImGuiWindowFlags_NoSavedSettings |  //
        ImGuiWindowFlags_NoTitleBar |       //
        ImGuiWindowFlags_NoMove |           //
        ImGuiWindowFlags_AlwaysAutoResize;

    ImGui::Begin("Tools", nullptr, window_flags);

    ImGui::Text("Select Tool:");
    ImGui::Spacing();

    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));

    ImGui::Button("Select entity", ImVec2(100, 0));
    ImGui::SameLine();
    ImGui::TextDisabled("(S)");
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
    ImGui::Button("Add voxel", ImVec2(100, 0));
    ImGui::PopStyleColor(2);

    ImGui::SameLine();
    ImGui::TextDisabled("(A)");
    ImGui::Spacing();

    ImGui::Button("Remove voxel", ImVec2(100, 0));

    ImGui::SameLine();
    ImGui::TextDisabled("(R)");
    ImGui::Spacing();

    ImGui::Button("Paint voxel", ImVec2(100, 0));

    ImGui::SameLine();
    ImGui::TextDisabled("(P)");
    ImGui::Spacing();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Active Tool:");
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", "Add Voxel");

    ImGui::PopStyleVar(1);

    ImGui::End();
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_TOOL_PANEL_INL_H
