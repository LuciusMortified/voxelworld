#pragma once

#ifndef VW_SCULPTOR_COLOR_PALETTE_PANEL_INL_H
#define VW_SCULPTOR_COLOR_PALETTE_PANEL_INL_H

namespace vw::sculptor {

inline color_palette_panel::color_palette_panel(
    app_state& st, const block_registry& registry
)
    : state_(&st), registry_(&registry) {}

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

    ImGuiWindowFlags window_flags =         //
        ImGuiWindowFlags_NoSavedSettings |  //
        ImGuiWindowFlags_NoMove |           //
        ImGuiWindowFlags_AlwaysAutoResize;

    ImGui::Begin("Block Palette", nullptr, window_flags);

    auto selected_color = registry_->get_color(state_->tool.selected_block);
    const ImVec4 selected_imvec4 = to_imvec4(selected_color);
    ImGui::ColorButton(
        "##current_color",
        selected_imvec4,
        ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoPicker,
        ImVec2(ImGui::GetContentRegionAvail().x, 40.0f)
    );
    ImGui::Text(
        "RGB: (%.2f, %.2f, %.2f)",
        selected_imvec4.x,
        selected_imvec4.y,
        selected_imvec4.z
    );
    ImGui::Text(
        "HEX: #%02X%02X%02X",
        selected_color.r(),
        selected_color.g(),
        selected_color.b()
    );

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    static const char* category_names[] = {"Terrain", "Character", "Metal"};

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    for (uint8 cat = 0; cat < 3; ++cat) {
        auto block_ids = registry_->blocks(static_cast<block_category>(cat));
        if (block_ids.empty()) continue;

        ImGui::PopStyleVar();
        if (cat > 0) ImGui::Spacing();
        ImGui::Text("%s", category_names[cat]);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

        int idx = 0;
        for (auto bid : block_ids) {
            auto clr = registry_->get_color(bid);
            const ImVec4 clr_imvec4 = to_imvec4(clr);

            ImGui::PushID(static_cast<int>(bid));

            constexpr ImGuiColorEditFlags btn_flags =  //
                ImGuiColorEditFlags_NoAlpha |          //
                ImGuiColorEditFlags_NoPicker |         //
                ImGuiColorEditFlags_NoBorder;

            if (ImGui::ColorButton("##block", clr_imvec4, btn_flags, ImVec2(30.0f, 30.0f))) {
                state_->tool.selected_block = bid;
            }

            ImGui::PopID();

            if (++idx % 6 != 0) {
                ImGui::SameLine(0, 0);
            }
        }
    }

    ImGui::PopStyleVar();

    state_->ui.left_bottom_voffset += ImGui::GetWindowHeight() + 10.f;

    ImGui::End();
}

inline auto color_palette_panel::to_imvec4(
    color clr
) -> ImVec4 {
    return {
        static_cast<float>(clr.r()) / 255.0f,
        static_cast<float>(clr.g()) / 255.0f,
        static_cast<float>(clr.b()) / 255.0f,
        1.0f
    };
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_COLOR_PALETTE_PANEL_INL_H
