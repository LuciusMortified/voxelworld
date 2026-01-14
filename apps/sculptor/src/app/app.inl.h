#pragma once

#ifndef VW_SCULPTOR_APP_INL_H
#define VW_SCULPTOR_APP_INL_H

#include "tools/add_voxel_tool.h"
#include "tools/dummy_tool.h"
#include "tools/remove_voxel_tool.h"
#include "tools/select_entity_tool.h"

namespace vw::sculptor {

inline app::app(
    gfx::engine<>& eng
)
    : gfx::app<>(eng)
    , camera_controller_(0.1f, 5.0f)

    , menu_bar_(eng, state_, op_manager_)
    , tool_panel_(state_)
    , color_palette_panel_(state_)
    , entity_properties_panel_(eng, state_, op_manager_)
    , entity_tree_panel_(eng, state_, op_manager_) {
    auto& window = eng.get_window();
    auto& camera = eng.get_camera();
    auto& renderer = eng.get_renderer();

    tools_[tools::select_entity] = std::make_unique<select_entity_tool<>>(eng, state_);
    tools_[tools::add_voxel]     = std::make_unique<add_voxel_tool<>>(eng, state_, op_manager_);
    tools_[tools::remove_voxel]  = std::make_unique<remove_voxel_tool<>>(eng, state_, op_manager_);
    tools_[tools::paint_voxel]   = std::make_unique<dummy_tool>();

    camera_controller_.setup(window, camera);

    window.sub<gfx::key_press_event>([this](const gfx::key_press_event& ev) {
        handle_key_press(ev);
        return true;
    });

    window.sub<gfx::window_close_event>([this](const gfx::window_close_event&) {
        get_engine().shutdown();
        return true;
    });

    window.sub<gfx::mouse_move_event>([this](const gfx::mouse_move_event& ev) {
        handle_mouse_move(ev);
        return true;
    });

    window.sub<gfx::mouse_press_event>([this](const gfx::mouse_press_event& ev) {
        handle_mouse_press(ev);
        return true;
    });

    window.sub<gfx::mouse_release_event>([this](const gfx::mouse_release_event& ev) {
        handle_mouse_release(ev);
        return true;
    });

    camera.set_position({0.0f, 0.0f, -15.0f});
    camera.set_rotation(0.0f, 0.0f);

    renderer.set_clear_color(vec4f{0.15f, 0.27f, 0.45f, 1.0f});
}

inline void app::render(
    float delta_time
) {
    camera_controller_.update(delta_time);

    auto& renderer = get_engine().get_renderer();
    renderer.draw_line(vec3f{0, 0, 0}, vec3f{100, 0, 0}, colors::blue);
    renderer.draw_line(vec3f{0, 0, 0}, vec3f{0, 100, 0}, colors::green);
    renderer.draw_line(vec3f{0, 0, 0}, vec3f{0, 0, 100}, colors::red);

    state_.ui.left_size_voffset  = 0.f;
    state_.ui.right_side_voffset = 0.f;

    menu_bar_.render(delta_time);

    // left side
    tool_panel_.render(delta_time);
    color_palette_panel_.render(delta_time);

    // right side
    entity_properties_panel_.render(delta_time);
    entity_tree_panel_.render(delta_time);

    tools_[state_.selected_tool]->render(delta_time);

    ImGui::Begin("Shadow Map Debug");
    void* shadow_map_texture_id = renderer.get_shadow_map_texture_id();
    ImGui::Image(
        reinterpret_cast<ImTextureID>(shadow_map_texture_id),
        ImVec2(512, 512),  // размер изображения
        ImVec2(0, 0),      // UV координаты верхнего левого угла
        ImVec2(1, 1),      // UV координаты нижнего правого угла
        ImVec4(1, 1, 1, 1), // tint цвет
        ImVec4(0, 0, 0, 0)  // border цвет
    );
    ImGui::End();
}

inline void app::handle_key_press(
    const gfx::key_press_event& ev
) {
    auto& io                  = ImGui::GetIO();
    bool really_want_keyboard = ImGui::IsAnyItemActive() || ImGui::IsAnyItemFocused();
    if (io.WantCaptureKeyboard && really_want_keyboard) {
        return;
    }

    if (camera_movement_enabled_) {
        return;
    }

    using keys = gfx::keyboard::keys;
    using mods = gfx::keyboard::mods;

    if (ev.key == keys::Z && ev.with(mods::CTRL) && !ev.with(mods::SHIFT)) {
        op_manager_.undo();
    }
    if (ev.key == keys::Z && ev.with(mods::CTRL) && ev.with(mods::SHIFT)) {
        op_manager_.redo();
    }
    if (ev.key == keys::S) {
        state_.selected_tool = tools::select_entity;
    }
    if (ev.key == keys::A) {
        state_.selected_tool = tools::add_voxel;
    }
    if (ev.key == keys::R) {
        state_.selected_tool = tools::remove_voxel;
    }
    if (ev.key == keys::P) {
        state_.selected_tool = tools::paint_voxel;
    }

    tools_[state_.selected_tool]->on_key_press(ev);
}

inline void app::handle_mouse_move(
    const gfx::mouse_move_event& ev
) {
    if (!camera_movement_enabled_) {
        tools_[state_.selected_tool]->on_mouse_move(ev);
    }
}

inline void app::handle_mouse_press(
    const gfx::mouse_press_event& ev
) {
    auto& io = ImGui::GetIO();
    if (io.WantCaptureMouse) {
        return;
    }

    if (!camera_movement_enabled_ && ev.button == gfx::mouse::buttons::RIGHT) {
        camera_movement_enabled_ = true;
        camera_controller_.set_mouse_captured(camera_movement_enabled_);
        camera_controller_.set_keyboard_control_enabled(camera_movement_enabled_);

        io.ConfigFlags |= ImGuiConfigFlags_NoMouse | ImGuiConfigFlags_NoKeyboard;

        ImGui::CloseCurrentPopup();
        ImGui::SetWindowFocus(nullptr);

        io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
    }

    if (!camera_movement_enabled_) {
        tools_[state_.selected_tool]->on_mouse_press(ev);
    }
}

inline void app::handle_mouse_release(
    const gfx::mouse_release_event& ev
) {
    auto& io = ImGui::GetIO();
    if (io.WantCaptureMouse) {
        return;
    }

    if (camera_movement_enabled_ && ev.button == gfx::mouse::buttons::RIGHT) {
        camera_movement_enabled_ = false;
        camera_controller_.set_mouse_captured(camera_movement_enabled_);
        camera_controller_.set_keyboard_control_enabled(camera_movement_enabled_);

        io.ConfigFlags &= ~(ImGuiConfigFlags_NoMouse | ImGuiConfigFlags_NoKeyboard);
    }

    if (!camera_movement_enabled_) {
        tools_[state_.selected_tool]->on_mouse_release(ev);
    }
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_APP_INL_H
