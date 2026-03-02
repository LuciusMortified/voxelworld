#pragma once

#ifndef VOXELWORLD_DEBUG_WINDOW_INL
#define VOXELWORLD_DEBUG_WINDOW_INL

#include <imgui.h>

#include "vw/gfx/debug/debug_window.h"
#include "vw/gfx/render/renderer.h"

namespace vw::gfx {

template <typename WC>
debug_window<WC>::debug_window(
    engine_type& engine
)
    : engine_(&engine) {}

template <typename WC>
void debug_window<WC>::render(
    [[maybe_unused]] float delta_time
) {
    if (!visible_) {
        return;
    }

    render_fps_window();
    if (show_systems_detail_) {
        render_systems_detail();
    }
    if (show_combined_buffers_detail_) {
        render_combined_buffers_detail();
    }
}

template <typename WC>
void debug_window<WC>::toggle_visibility() {
    visible_ = !visible_;
}

template <typename WC>
void debug_window<WC>::set_visible(
    bool visible
) {
    visible_ = visible;
}
template <typename WC>
bool debug_window<WC>::is_visible() const {
    return visible_;
}

template <typename WC>
void debug_window<WC>::render_fps_window() {
    ImGuiWindowFlags window_flags =          //
        ImGuiWindowFlags_NoResize |          //
        ImGuiWindowFlags_NoScrollbar |       //
        ImGuiWindowFlags_NoCollapse |        //
        ImGuiWindowFlags_AlwaysAutoResize |  //
        ImGuiWindowFlags_NoSavedSettings;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 window_pos =
        ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - 10, viewport->WorkPos.y + 10);
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Appearing, ImVec2(1.0f, 0.0f));

    if (ImGui::Begin("Debug Tool", &visible_, window_flags)) {
        const auto& eng_stats = engine_->get_stats();
        ImGui::Text("frame_ms  %.2fms fps %.0f", eng_stats.frame_ms, eng_stats.fps);
        ImGui::Text("update_ms %.2fms", eng_stats.world_update_ms);
        ImGui::Text("render_ms %.2fms", eng_stats.world_render_ms);

        ImGui::Text(
            "RAM %.2fMB VRAM %.2fMB",
            eng_stats.ram_usage_bytes / (1024.0f * 1024.0f),
            eng_stats.vram_usage_bytes / (1024.0f * 1024.0f)
        );

        const auto& rend_stats = engine_->get_renderer().get_stats();
        ImGui::Text("draw calls %u", rend_stats.draw_call_count);

        const auto& cb_stats = rend_stats.combined_buffers;
        ImGui::Text(
            "vertex load avg %.2f min %.2f max %.2f",
            cb_stats.vertex_load_avg,
            cb_stats.vertex_load_min,
            cb_stats.vertex_load_max
        );
        ImGui::Text(
            "index load  avg %.2f min %.2f max %.2f",
            cb_stats.index_load_avg,
            cb_stats.index_load_min,
            cb_stats.index_load_max
        );
        ImGui::Text(
            "mesh %u/%u instance %u/%u",
            cb_stats.mesh_count,
            cb_stats.mesh_capacity,
            cb_stats.instance_count,
            cb_stats.instance_capacity
        );
        ImGui::Spacing();
        if (ImGui::Button("systems")) {
            show_systems_detail_ = !show_systems_detail_;
        }
        ImGui::SameLine();
        if (ImGui::Button("buffers")) {
            show_combined_buffers_detail_ = !show_combined_buffers_detail_;
        }
        ImGui::Spacing();
        ImGui::Separator();

        render_render_mode_controls();
    }
    ImGui::End();
}

template <typename WC>
void debug_window<WC>::render_render_mode_controls() const {
    ImGui::Spacing();
    if (ImGui::Button("lit")) {
        engine_->get_renderer().set_render_mode(render_mode::lit);
    }
    ImGui::SameLine();
    if (ImGui::Button("wire")) {
        engine_->get_renderer().set_render_mode(render_mode::wireframe);
    }
}

template <typename WC>
void debug_window<WC>::render_systems_detail() {
    ImGuiWindowFlags window_flags =          //
        ImGuiWindowFlags_NoCollapse |        //
        ImGuiWindowFlags_NoSavedSettings |   //
        ImGuiWindowFlags_AlwaysAutoResize |  //
        ImGuiWindowFlags_NoFocusOnAppearing;

    bool show = show_systems_detail_;
    if (ImGui::Begin("Debug Tool - Systems", &show, window_flags)) {
        show_systems_detail_ = show;
        const auto& s = engine_->get_stats().systems;

        auto row = [](const char* name, float32 ms) {
            ImVec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
            if (ms > 5.0f) color = {1.0f, 0.3f, 0.3f, 1.0f};
            else if (ms > 1.0f) color = {1.0f, 0.8f, 0.2f, 1.0f};
            ImGui::TextColored(color, "%-14s %6.2f ms", name, ms);
        };

        row("transform", s.transform_ms);
        row("model", s.model_ms);

        const auto& ms = engine_->get_world().get_model_system().get_stats();
        ImGui::Indent();
        row("process_done", ms.process_completed_ms);
        row("update_done", ms.update_completed_ms);
        row("process_dirty", ms.process_dirty_ms);
        ImGui::Text("pending ent:%u mesh:%u", ms.pending_entities_count, ms.pending_meshes_count);
        ImGui::Unindent();

        row("spatial", s.spatial_ms);
        row("light", s.light_ms);
        row("world_grid", s.world_grid_ms);
        row("animation", s.animation_ms);
    }
    ImGui::End();
}

template <typename WC>
void debug_window<WC>::render_combined_buffers_detail() {
    ImGuiWindowFlags window_flags =          //
        ImGuiWindowFlags_NoCollapse |        //
        ImGuiWindowFlags_NoSavedSettings |   //
        ImGuiWindowFlags_AlwaysAutoResize |  //
        ImGuiWindowFlags_NoFocusOnAppearing;

    bool show_detail = show_combined_buffers_detail_;
    if (ImGui::Begin("Debug Tool - Combined Buffers", &show_detail, window_flags)) {
        show_combined_buffers_detail_ = show_detail;
        const auto& rend_stats        = engine_->get_renderer().get_stats();
        const auto& buffers           = rend_stats.combined_buffers.buffers;

        for (size_t i = 0; i < buffers.size(); ++i) {
            const auto& buffer = buffers[i];
            ImGui::Text("Buffer %zu:", i);
            ImGui::Indent();
            ImGui::Text(
                "chunk_size: vertex=%u index=%u",
                buffer.chunk_size.vertex_count,
                buffer.chunk_size.index_count
            );
            ImGui::Text(
                "vertex load: avg %.2f min %.2f max %.2f",
                buffer.vertex_load_avg,
                buffer.vertex_load_min,
                buffer.vertex_load_max
            );
            ImGui::Text(
                "index load: avg %.2f min %.2f max %.2f",
                buffer.index_load_avg,
                buffer.index_load_min,
                buffer.index_load_max
            );
            ImGui::Text(
                "mesh %u/%u instance %u/%u",
                buffer.mesh_count,
                buffer.mesh_capacity,
                buffer.instance_count,
                buffer.instance_capacity
            );
            ImGui::Unindent();
            ImGui::Separator();
        }
    }
    ImGui::End();
}

}  // namespace vw::gfx

#endif  // VOXELWORLD_DEBUG_WINDOW_INL
