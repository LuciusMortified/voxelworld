module;

#include <imgui.h>

module vw.sculptor;

import std;

import vw.core;
import vw.ecs;
import vw.world;
import vw.platform;
import vw.gfx;

namespace vw::sculptor {
namespace {

auto to_imvec4(color clr) -> ImVec4 {
    return {
        static_cast<float>(clr.r()) / 255.0f,
        static_cast<float>(clr.g()) / 255.0f,
        static_cast<float>(clr.b()) / 255.0f,
        1.0f
    };
}

}  // namespace


color_palette_panel::color_palette_panel(
    app_state& st, const block_registry& registry
)
    : state_(&st), registry_(&registry) {}

void color_palette_panel::render(
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

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    uint8 idx = 0;
    for (const auto block : registry_->blocks()) {
        if (block.id == blocks::air) {
            continue;
        }

        ImGui::PushID(idx);

        constexpr ImGuiColorEditFlags btn_flags =  //
            ImGuiColorEditFlags_NoAlpha |          //
            ImGuiColorEditFlags_NoPicker |         //
            ImGuiColorEditFlags_NoBorder;

        const ImVec4 clr_imvec4 = to_imvec4(block.clr);
        if (ImGui::ColorButton("##block", clr_imvec4, btn_flags, ImVec2(30.0f, 30.0f))) {
            state_->tool.selected_block = block.id;
        }

        ImGui::PopID();

        if (++idx % 6 != 0) {
            ImGui::SameLine(0, 0);
        }
    }

    ImGui::PopStyleVar();

    state_->ui.left_bottom_voffset += ImGui::GetWindowHeight() + 10.f;

    ImGui::End();
}


}  // namespace vw::sculptor
