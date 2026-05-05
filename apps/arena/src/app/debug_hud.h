#pragma once

#ifndef VW_ARENA_DEBUG_HUD_H
#define VW_ARENA_DEBUG_HUD_H

#include <imgui.h>

#include <vw/gfx.h>

#include "vw/gfx/camera/third_person_camera_controller.h"
#include "vw/ecs/systems/world_grid/world_grid.h"

#include "player.h"

namespace vw::arena {

inline auto render_debug_hud(
    const gfx::engine<>& engine,
    const player& player,
    const gfx::third_person_camera_controller<>& camera_controller,
    bool show_colliders
) -> void {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const auto window_pos         = ImVec2(viewport->WorkPos.x + 10, viewport->WorkPos.y + 10);
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(0.0f, 0.0f));

    constexpr ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoFocusOnAppearing;

    ImGui::Begin("Arena", nullptr, window_flags);
    ImGui::Text("Controls:");
    ImGui::Text("WASD - move");
    ImGui::Text("SPACE - jump");
    ImGui::Text("1 - toggle sword");
    ImGui::Text("LMB - sword attack");
    ImGui::Text("F - test impulse");
    ImGui::Text("Mouse - rotate camera");
    ImGui::Text("Scroll - zoom");
    ImGui::Text("F1 - toggle cursor");
    ImGui::Text("F2 - toggle colliders");
    ImGui::Text("ESC - exit");
    ImGui::Separator();

    if (player.is_placed()) {
        auto& world           = engine.get_world();
        const auto player_ent = player.get_entity();
        const auto& tc        = world.get<gfx::transform_component>(player_ent);
        const auto pos        = tc.get_position();
        ImGui::Text("Position: (%.1f, %.1f, %.1f)", pos.x, pos.y, pos.z);

        const auto& rb = world.get<gfx::rigid_body_component>(player_ent);
        const auto vel = rb.get_velocity();
        ImGui::Text("Velocity: (%.1f, %.1f, %.1f)", vel.x, vel.y, vel.z);
        const auto imp = rb.get_impulse();
        ImGui::Text("Impulse: (%.1f, %.1f, %.1f)", imp.x, imp.y, imp.z);
        ImGui::Text("Grounded: %s", rb.is_grounded() ? "yes" : "no");
        ImGui::Text("Frozen: %s", rb.is_frozen() ? "yes" : "no");

        ImGui::Text("Arm length: %.1f", camera_controller.get_actual_arm_length());

        ImGui::Separator();
        ImGui::Text("Animation FSM:");
        const auto& fsm_comp = world.get<gfx::animation_fsm_component>(player_ent);
        for (size_t i = 0; i < fsm_comp.machine_count(); ++i) {
            const auto& state = fsm_comp.get_machine(i).get_current_state();
            ImGui::Text("  Layer %zu: %s", i, state.c_str());
        }

        ImGui::Separator();
        ImGui::Text("Sword: %s", player.has_sword() ? "equipped" : "none");
        ImGui::Text("Colliders: %s", show_colliders ? "visible" : "hidden");
    }

    const auto& wgs = engine.get_world().template system<gfx::world_grid_system>();
    if (const auto* grid = wgs.grid()) {
        ImGui::Separator();
        ImGui::Text("Loaded chunks: %u", grid->chunk_count());
        ImGui::Text("Pending columns: %u", wgs.get_stats().pending_count);
    }

    ImGui::End();
}

}  // namespace vw::arena

#endif  // VW_ARENA_DEBUG_HUD_H
