#pragma once

#ifndef VW_SCULPTOR_COLOR_PALETTE_PANEL_INL_H
#define VW_SCULPTOR_COLOR_PALETTE_PANEL_INL_H

namespace vw::sculptor {

inline color_palette_panel::color_palette_panel(
    app_state& st
)
    : state_(&st) {}

inline void color_palette_panel::render(
    float delta_time
) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 window_pos       = ImVec2(
        viewport->WorkPos.x + 10,
        viewport->WorkPos.y + viewport->WorkSize.y  //
            - state_->ui.left_bottom_voffset        //
            - 10
    );
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(0.0f, 1.0f));

    ImGuiWindowFlags window_flags =  //
                                     // ImGuiWindowFlags_NoCollapse |       //
        ImGuiWindowFlags_NoSavedSettings |  //
        // ImGuiWindowFlags_NoTitleBar |       //
        ImGuiWindowFlags_NoMove |  //
        ImGuiWindowFlags_AlwaysAutoResize;

    ImGui::Begin("Color Palette", nullptr, window_flags);

    const ImVec4 selected_color_imvec4 = to_imvec4(state_->selected_color);
    ImGui::ColorButton(
        "##current_color",
        selected_color_imvec4,
        ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoPicker,
        ImVec2(ImGui::GetContentRegionAvail().x, 40.0f)
    );
    ImGui::Text(
        "RGB: (%.2f, %.2f, %.2f)",
        selected_color_imvec4.x,
        selected_color_imvec4.y,
        selected_color_imvec4.z
    );
    ImGui::Text(
        "HEX: #%02X%02X%02X",
        state_->selected_color.r(),
        state_->selected_color.g(),
        state_->selected_color.b()
    );

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    auto idx = 0;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    for (auto clr : colors::all) {
        const ImVec4 clr_imvec4 = to_imvec4(clr);

        auto btn_id = std::format("##{}", clr.value);
        ImGui::PushID(btn_id.c_str());

        ImGuiColorEditFlags btn_flags =     //
            ImGuiColorEditFlags_NoAlpha |   //
            ImGuiColorEditFlags_NoPicker |  //
            ImGuiColorEditFlags_NoBorder;

        auto color_picked =
            ImGui::ColorButton("##color_button", clr_imvec4, btn_flags, ImVec2(30.0f, 30.0f));
        if (color_picked) {
            state_->selected_color = clr;
        }

        ImGui::PopID();

#if 0
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("%s", clr.name.c_str());
            ImGui::EndTooltip();
        }
#endif

        if (++idx % 6 != 0) {
            ImGui::SameLine(0, 0);
        }
    }

    ImGui::PopStyleVar();

    state_->ui.left_bottom_voffset += ImGui::GetWindowHeight() + 10.f;

    ImGui::End();
}

inline ImVec4 color_palette_panel::to_imvec4(
    const color& clr
) {
    return {
        static_cast<float>(clr.r()) / 255.0f,
        static_cast<float>(clr.g()) / 255.0f,
        static_cast<float>(clr.b()) / 255.0f,
        1.0f
    };
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_COLOR_PALETTE_PANEL_INL_H
