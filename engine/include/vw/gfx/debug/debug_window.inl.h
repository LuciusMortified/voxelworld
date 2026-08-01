#pragma once

#ifndef VOXELWORLD_DEBUG_WINDOW_INL
#define VOXELWORLD_DEBUG_WINDOW_INL

#include <algorithm>
#include <array>
#include <cfloat>
#include <cstdio>
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
    if (show_render_detail_) {
        render_render_detail();
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

        const float32 frame_ms      = eng_stats.frame_ms;
        fps_bucket_current_max_     = std::max(fps_bucket_current_max_, frame_ms);
        fps_bucket_current_min_     = std::min(fps_bucket_current_min_, frame_ms);
        fps_bucket_accum_ms_       += frame_ms;
        if (fps_bucket_accum_ms_ >= fps_bucket_duration_ms_) {
            fps_bucket_max_[fps_bucket_index_] = fps_bucket_current_max_;
            fps_bucket_min_[fps_bucket_index_] = fps_bucket_current_min_;
            fps_bucket_index_                  = (fps_bucket_index_ + 1) % fps_bucket_count_;
            if (fps_bucket_filled_count_ < fps_bucket_count_) {
                ++fps_bucket_filled_count_;
            }
            fps_bucket_accum_ms_    = 0.0f;
            fps_bucket_current_max_ = 0.0f;
            fps_bucket_current_min_ = FLT_MAX;
        }

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
        {
            float32 window_min = FLT_MAX;
            float32 window_max = 0.0f;
            for (size_t i = 0; i < fps_bucket_filled_count_; ++i) {
                window_min = std::min(window_min, fps_bucket_min_[i]);
                window_max = std::max(window_max, fps_bucket_max_[i]);
            }
            if (fps_bucket_filled_count_ == 0) {
                window_min = 0.0f;
            }

            std::array<char, 96> overlay{};
            std::snprintf(
                overlay.data(), overlay.size(),
                "peaks 10s  min %.2f max %.2f ms", window_min, window_max
            );

            int plot_count  = static_cast<int>(fps_bucket_filled_count_);
            int plot_offset = (fps_bucket_filled_count_ < fps_bucket_count_)
                ? 0
                : static_cast<int>(fps_bucket_index_);

            ImGui::PlotLines(
                "##frame_ms_peaks",
                fps_bucket_max_.data(),
                plot_count,
                plot_offset,
                overlay.data(),
                0.0f,
                FLT_MAX,
                ImVec2(280.0f, 50.0f)
            );
        }
        ImGui::Spacing();
        if (ImGui::Button("systems")) {
            show_systems_detail_ = !show_systems_detail_;
        }
        ImGui::SameLine();
        if (ImGui::Button("render")) {
            show_render_detail_ = !show_render_detail_;
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
        ImGui::Text("(system metrics — to be reimplemented)");
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

template <typename WC>
void debug_window<WC>::render_render_detail() {
    ImGuiWindowFlags window_flags =          //
        ImGuiWindowFlags_NoCollapse |        //
        ImGuiWindowFlags_NoSavedSettings |   //
        ImGuiWindowFlags_AlwaysAutoResize |  //
        ImGuiWindowFlags_NoFocusOnAppearing;

    bool show = show_render_detail_;
    if (ImGui::Begin("Debug Tool - Render", &show, window_flags)) {
        show_render_detail_ = show;
        const auto& t = engine_->get_renderer().get_stats().timing;
        const auto& eng = engine_->get_stats();

        auto row = [this](const char* name, float32 ms) {
            auto& max_val = metric_max_[name];
            if (ms > max_val) {
                max_val = ms;
            }

            ImVec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
            if (ms > 5.0f) {
                color = {1.0f, 0.3f, 0.3f, 1.0f};
            } else if (ms > 1.0f) {
                color = {1.0f, 0.8f, 0.2f, 1.0f};
            }
            ImGui::TextColored(
                color, "%-14s %6.2f ms  max %5.2f ms", name, ms, max_val
            );
        };

        row("begin_frame", eng.begin_frame_ms);
        row("app_render", eng.app_render_ms);
        row("shadow_update", t.shadow_map_update_ms);
        row("buffer_pool", t.buffer_pool_update_ms);

        const auto& bp = engine_->get_renderer().get_stats().combined_buffers.timing;
        ImGui::Indent();
        row("destroyed", bp.destroyed_ms);
        row("meshes", bp.meshes_ms);
        row("transforms", bp.transforms_ms);
        row("staging", bp.staging_flush_ms);
        ImGui::Unindent();

        row("compute_cull", t.compute_cull_ms);
        row("shadow_pass", t.shadow_pass_ms);
        row("world_pass", t.world_pass_ms);
        ImGui::Indent();
        row("uniform", t.world_pass_uniform_ms);
        row("geometry", t.world_pass_geometry_ms);
        row("debug", t.world_pass_debug_ms);
        row("imgui", t.world_pass_imgui_ms);
        ImGui::Unindent();
        row("end_frame", eng.end_frame_ms);

        ImGui::Spacing();
        if (ImGui::Button("reset##render")) {
            metric_max_.clear();
        }
    }
    ImGui::End();
}

}  // namespace vw::gfx

#endif  // VOXELWORLD_DEBUG_WINDOW_INL
