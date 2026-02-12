#pragma once

#ifndef VW_SCULPTOR_APP_INL_H
#define VW_SCULPTOR_APP_INL_H

#include <filesystem>

#include "tools/add_voxel_tool.h"
#include "tools/paint_tool.h"
#include "tools/remove_voxel_tool.h"

namespace vw::sculptor {

inline app::app(
    engine_type& eng
)
    : gfx::app<>(eng)
    , camera_controller_(0.1f, 5.0f)

    , menu_bar_(eng, state_, op_manager_)
    , tool_panel_(state_)
    , color_palette_panel_(state_)
    , entity_properties_panel_(eng, state_, op_manager_)
    , entity_tree_panel_(eng, state_, op_manager_)
    , startup_modal_(eng, state_)
    , new_file_modal_(eng, state_)
    , open_file_modal_(eng, state_)
    , save_as_modal_(eng, state_) {

    init_asset_dir_();

    auto& window   = eng.get_window();
    auto& camera   = eng.get_camera();
    auto& renderer = eng.get_renderer();

    tools_[tools::add_voxel]     = std::make_unique<add_voxel_tool>(eng, state_, op_manager_);
    tools_[tools::remove_voxel]  = std::make_unique<remove_voxel_tool>(eng, state_, op_manager_);
    tools_[tools::paint_voxel]   = std::make_unique<paint_tool>(eng, state_, op_manager_);

    camera_controller_.setup(window, camera);
    camera_controller_.set_camera_speed(15.f);

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

    auto& dir_light_settings     = renderer.get_directional_light_settings();
    dir_light_settings.direction = math::normalize(vec3f{+0.3f, -1.0f, +0.3f});
}

inline void app::render(
    float delta_time
) {
    if (state_.selected_tool != active_tool_) {
        active_tool_ = state_.selected_tool;
        tools_[active_tool_]->on_activate();
    }

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

    // modals
    startup_modal_.render(delta_time);
    new_file_modal_.render(delta_time);
    open_file_modal_.render(delta_time);
    save_as_modal_.render(delta_time);

    tools_[active_tool_]->render(delta_time);

    if (state_.has_unsaved_changes != prev_unsaved_state_) {
        prev_unsaved_state_ = state_.has_unsaved_changes;
        update_title_();
    }

#if 1
    ImGui::Begin("Shadow Map Debug");
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    // Сетка 2x2 для всех каскадов
    for (uint32 i = 0; i < gfx::shadow_map::cascade_count; ++i) {
        void* shadow_map_texture_id = renderer.get_shadow_map_texture_id(i);
        ImGui::Image(
            reinterpret_cast<ImTextureID>(shadow_map_texture_id),
            ImVec2(256, 256),  // Уменьшенный размер для сетки
            ImVec2(0, 0),      // UV координаты верхнего левого угла
            ImVec2(1, 1),      // UV координаты нижнего правого угла
            ImVec4(1, 1, 1, 1), // tint цвет
            ImVec4(0, 0, 0, 0)  // border цвет
        );
        if (i % 2 == 0) {
            ImGui::SameLine();
        }
    }
    ImGui::PopStyleVar();
    ImGui::End();
#endif
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

    if (ev.key == keys::KEY_1) {
        state_.selected_tool = tools::add_voxel;
    }
    if (ev.key == keys::KEY_2) {
        state_.selected_tool = tools::remove_voxel;
    }
    if (ev.key == keys::KEY_3) {
        state_.selected_tool = tools::paint_voxel;
    }

    tools_[active_tool_]->on_key_press(ev);

    if (ev.key == keys::N && ev.with(mods::CTRL)) {
        state_.ui.need_new_file_modal = true;
    }
    if (ev.key == keys::O && ev.with(mods::CTRL)) {
        state_.ui.need_open_file_modal = true;
    }
    if (ev.key == keys::S && ev.with(mods::CTRL) && !ev.with(mods::SHIFT)) {
        gfx::vox_serializer serializer{
            get_engine().get_world(),
            state_.name_to_entity[state_.root_name],
            state_.entity_to_name
        };

        namespace fs = std::filesystem;
        fs::path assets_dir_path{app_state::asset_dir_name};
        fs::path filepath{assets_dir_path / state_.filename};
        serializer.serialize(filepath);
        state_.has_unsaved_changes = false;
    }
    if (ev.key == keys::S && ev.with(mods::CTRL) && ev.with(mods::SHIFT)) {
        state_.ui.need_save_as_modal = true;
    }
}

inline void app::handle_mouse_move(
    const gfx::mouse_move_event& ev
) {
    if (!camera_movement_enabled_) {
        tools_[active_tool_]->on_mouse_move(ev);
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
        tools_[active_tool_]->on_mouse_press(ev);
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
        tools_[active_tool_]->on_mouse_release(ev);
    }
}

inline void app::update_title_() {
    std::string title;
    if (state_.filename.empty()) {
        title = std::format("Sculptor {}", version_string);
    } else if (state_.has_unsaved_changes) {
        title = std::format("Sculptor {} | {} | UNSAVED CHANGES", version_string, state_.filename);
    } else {
        title = std::format("Sculptor {} | {}", version_string, state_.filename);
    }
    get_engine().get_window().set_title(title);
}

inline void app::init_asset_dir_() {
    if (!std::filesystem::exists(app_state::asset_dir_name)) {
        std::error_code ec;
        std::filesystem::create_directories(app_state::asset_dir_name, ec);
        if (ec) {
            log::critical(
                "Failed to create asset directory '{}': {}",  //
                app_state::asset_dir_name,
                ec.message()
            );
        }
    }
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_APP_INL_H
