#pragma once

#ifndef VW_SCULPTOR_UI_UTILS_H
#define VW_SCULPTOR_UI_UTILS_H
#include <imgui.h>


namespace vw::sculptor {

inline void imgui_input_text_string(
    std::string_view label, std::string& value
) {
    constexpr size_t max_length = 64;
    std::array<char, max_length> buffer{};

#ifdef _WIN32
    strncpy_s(buffer.data(), max_length, value.data(), max_length - 1);
#else
    std::strncpy(buffer.data(), value.data(), max_length - 1);
#endif

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label.data(), label.data() + label.size());
    ImGui::SameLine();

    const auto hidden_label = std::format("##{}", label);
    if (ImGui::InputText(hidden_label.c_str(), buffer.data(), max_length)) {
        value = std::string{buffer.data()};
    }
}

inline auto imgui_input_int_left(
    std::string_view label, int* value
) -> bool {
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label.data(), label.data() + label.size());
    ImGui::SameLine();

    const auto hidden_label = std::format("##{}", label);
    return ImGui::InputInt(hidden_label.c_str(), value);
}

inline void imgui_clamp_window_pos_to_viewport() {
    if (!ImGui::IsWindowCollapsed() && !ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        const ImVec2 window_pos       = ImGui::GetWindowPos();
        const ImVec2 window_size      = ImGui::GetWindowSize();

        const auto viewport_pos  = ImVec2{viewport->WorkPos.x + 10, viewport->WorkPos.y + 10};
        const auto viewport_size = ImVec2{viewport->WorkSize.x - 20, viewport->WorkSize.y - 20};

        const float new_x = std::max(
            viewport_pos.x, std::min(window_pos.x, viewport_pos.x + viewport_size.x - window_size.x)
        );
        const float new_y = std::max(
            viewport_pos.y, std::min(window_pos.y, viewport_pos.y + viewport_size.y - window_size.y)
        );

        if (new_x != window_pos.x || new_y != window_pos.y) {
            ImGui::SetWindowPos(ImVec2(new_x, new_y));
        }
    }
}

inline auto imgui_drag_vec3f(std::string_view label, vec3f& vec, float label_offset = 60.f) -> bool {
    bool changed = false;

    const auto field_id = std::format("##drag_vec3f_{}", label);
    ImGui::PushID(field_id.c_str());

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label.data(), label.data() + label.size());
    ImGui::SameLine(label_offset);

    ImGui::PushItemWidth(80.0f);
    changed |= ImGui::DragFloat(std::format("##{}X", label).c_str(), &vec.x, 0.1f, 0, 0, "%.4f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    ImGui::PushItemWidth(80.0f);
    changed |= ImGui::DragFloat(std::format("##{}Y", label).c_str(), &vec.y, 0.1f, 0, 0, "%.4f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    ImGui::PushItemWidth(80.0f);
    changed |= ImGui::DragFloat(std::format("##{}Z", label).c_str(), &vec.z, 0.1f, 0, 0, "%.4f");
    ImGui::PopItemWidth();

    ImGui::PopID();

    return changed;
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_UI_UTILS_H
