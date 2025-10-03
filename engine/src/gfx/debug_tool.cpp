#include "vw/gfx/debug_tool.h"

#include <algorithm>
#include <numeric>

#include <imgui.h>

namespace vw::gfx {

debug_tool::debug_tool(renderer& renderer)
    : renderer_(&renderer),
      last_fps_update_(std::chrono::high_resolution_clock::now()) {
    fps_history_.reserve(MAX_HISTORY_SIZE);
}

debug_tool::~debug_tool() = default;

void debug_tool::update(float delta_time) {
    fps_accumulator_ += delta_time;
    frame_count_++;

    const auto current_time = std::chrono::high_resolution_clock::now();
    const auto time_since_update =
        std::chrono::duration<float>(current_time - last_fps_update_).count();

    if (time_since_update >= FPS_UPDATE_INTERVAL) {
        current_fps_ = static_cast<float>(frame_count_) / fps_accumulator_;
        update_fps_history(current_fps_);

        fps_accumulator_ = 0.0f;
        frame_count_ = 0;
        last_fps_update_ = current_time;
    }
}

void debug_tool::render() {
    if (!visible_) {
        return;
    }

    render_fps_window();
}

void debug_tool::toggle_visibility() {
    visible_ = !visible_;
}

void debug_tool::update_fps_history(float fps) {
    fps_history_.push_back(fps);

    if (fps_history_.size() > MAX_HISTORY_SIZE) {
        fps_history_.erase(fps_history_.begin());
    }

    if (!fps_history_.empty()) {
        average_fps_ =
            std::accumulate(fps_history_.begin(), fps_history_.end(), 0.0f) /
            static_cast<float>(fps_history_.size());
        min_fps_ = *std::ranges::min_element(fps_history_);
        max_fps_ = *std::ranges::max_element(fps_history_);
    }
}

void debug_tool::render_fps_window() {
    // clang-format off
    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoMove;
    // clang-format on

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 window_pos = ImVec2(
        viewport->WorkPos.x + viewport->WorkSize.x - 10,
        viewport->WorkPos.y + 10
    );
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(1.0f, 0.0f));

    if (ImGui::Begin("Debug Tool", &visible_, window_flags)) {
        ImGui::TextColored(
            ImVec4(0.0f, 1.0f, 0.0f, 1.0f),
            "FPS: %.1f (%.3f ms/frame)",
            current_fps_,
            1000.0f / current_fps_
        );
        ImGui::Text(
            "Avg: %.1f Min: %.1f Max: %.1f",
            average_fps_,
            min_fps_,
            max_fps_
        );

        render_fps_graph();

        ImGui::Separator();

        render_render_mode_controls();
    }
    ImGui::End();
}

void debug_tool::render_fps_graph() const {
    if (fps_history_.empty()) {
        return;
    }

    float min_val = std::max(0.f, min_fps_ - 10.f);
    float max_val = std::min(5000.f, max_fps_ + 10.f);

    ImGui::PlotLines(
        "##FPSGraph",
        fps_history_.data(),
        static_cast<int>(fps_history_.size()),
        0,
        nullptr,
        min_val,
        max_val,
        ImVec2(250, 80)
    );
}

void debug_tool::render_render_mode_controls() const {
    ImGui::Text("Render Mode:");

    const char* mode_names[] = {"Lit", "Wireframe"};
    int current_mode = static_cast<int>(renderer_->get_render_mode());

    if (ImGui::Combo("##RenderMode", &current_mode, mode_names, 2)) {
        renderer_->set_render_mode(static_cast<render_mode>(current_mode));
    }
}

}  // namespace vw::gfx
