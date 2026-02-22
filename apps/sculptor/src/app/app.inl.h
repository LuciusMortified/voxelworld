#pragma once

#ifndef VW_SCULPTOR_APP_INL_H
#define VW_SCULPTOR_APP_INL_H

#include <filesystem>

#include "operations/add_keyframe_operation.h"
#include "operations/add_track_operation.h"
#include "operations/remove_keyframe_operation.h"
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
    , socket_panel_(eng, state_, op_manager_)
    , keyframe_properties_panel_(eng, state_, op_manager_)
    , entity_tree_panel_(eng, state_, op_manager_)
    , clip_manager_panel_(eng, state_, op_manager_)
    , timeline_panel_(eng, state_, op_manager_)
    , startup_modal_(eng, state_)
    , new_file_modal_(eng, state_)
    , open_file_modal_(eng, state_)
    , save_as_modal_(eng, state_) {
    init_asset_dir_();

    auto& window   = eng.get_window();
    auto& camera   = eng.get_camera();
    auto& renderer = eng.get_renderer();

    tools_[tools::add_voxel]    = std::make_unique<add_voxel_tool>(eng, state_, op_manager_);
    tools_[tools::remove_voxel] = std::make_unique<remove_voxel_tool>(eng, state_, op_manager_);
    tools_[tools::paint_voxel]  = std::make_unique<paint_tool>(eng, state_, op_manager_);

    camera_controller_.setup(window, camera);
    camera_controller_.set_camera_speed(15.f);

    window.sub<gfx::key_press_event>([this](const gfx::key_press_event& ev) -> bool {
        handle_key_press(ev);
        return true;
    });

    window.sub<gfx::window_close_event>([this](const gfx::window_close_event&) -> bool {
        get_engine().shutdown();
        return true;
    });

    window.sub<gfx::mouse_move_event>([this](const gfx::mouse_move_event& ev) -> bool {
        handle_mouse_move(ev);
        return true;
    });
    window.sub<gfx::mouse_press_event>([this](const gfx::mouse_press_event& ev) -> bool {
        handle_mouse_press(ev);
        return true;
    });
    window.sub<gfx::mouse_release_event>([this](const gfx::mouse_release_event& ev) -> bool {
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

    state_.ui.left_top_voffset    = 0.f;
    state_.ui.left_bottom_voffset = 0.f;
    state_.ui.right_top_voffset   = 0.f;

    menu_bar_.render(delta_time);

    // left side
    tool_panel_.render(delta_time);

    if (state_.ui.show_timeline) {
        timeline_panel_.render(delta_time);
    }
    color_palette_panel_.render(delta_time);

    // right side
    entity_properties_panel_.render(delta_time);
    entity_tree_panel_.render(delta_time);
    if (state_.ui.show_sockets) {
        socket_panel_.render(delta_time);
    }
    if (state_.ui.need_create_clip_modal || state_.ui.need_load_clip_modal) {
        state_.ui.show_clip_manager = true;
    }
    if (state_.ui.show_clip_manager) {
        clip_manager_panel_.render(delta_time);
    }
    keyframe_properties_panel_.render(delta_time);

    // modals
    startup_modal_.render(delta_time);
    new_file_modal_.render(delta_time);
    open_file_modal_.render(delta_time);
    save_as_modal_.render(delta_time);

    handle_animation_actions_();

    tools_[active_tool_]->render(delta_time);

    if (state_.has_unsaved_changes != prev_unsaved_state_ ||
        state_.selected_clip_name != prev_clip_name_ ||
        state_.has_any_unsaved_clip() != prev_clip_unsaved_state_) {
        prev_unsaved_state_      = state_.has_unsaved_changes;
        prev_clip_name_          = state_.selected_clip_name;
        prev_clip_unsaved_state_ = state_.has_any_unsaved_clip();
        update_title_();
    }

#if 0
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
    const auto& io = ImGui::GetIO();

    const bool really_want_keyboard =  //
        ImGui::IsAnyItemActive() || ImGui::IsAnyItemFocused();
    if (io.WantCaptureKeyboard && really_want_keyboard || camera_movement_enabled_) {
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

    if (ev.key == keys::S && ev.with(mods::ALT)) {
        state_.ui.show_sockets ^= true;
    }
    if (ev.key == keys::A && ev.with(mods::ALT)) {
        state_.ui.show_clip_manager ^= true;
    }
    if (ev.key == keys::T && ev.with(mods::ALT)) {
        state_.ui.show_timeline ^= true;
    }
    if (ev.key == keys::SPACE && !ev.with(mods::CTRL)) {
        state_.need_toggle_playback = true;
    }
    if (ev.key == keys::LEFT && !ev.with(mods::CTRL) && state_.ui.show_timeline) {
        state_.need_step_backward = true;
    }
    if (ev.key == keys::RIGHT && !ev.with(mods::CTRL) && state_.ui.show_timeline) {
        state_.need_step_forward = true;
    }

    handle_file_shortcuts(ev);
}

inline void app::handle_file_shortcuts(
    const gfx::key_press_event& ev
) {
    using keys = gfx::keyboard::keys;
    using mods = gfx::keyboard::mods;

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
            {.entity_names = state_.entity_to_name, .excluded = state_.get_preview_entities()}
        };

        namespace fs = std::filesystem;
        const fs::path assets_dir_path{app_state::asset_dir_name};
        const fs::path filepath{assets_dir_path / state_.filename};

        auto result = serializer.serialize(filepath);
        // TODO: handle serialize errors

        clip_manager_panel_.save_all_clips();

        state_.has_unsaved_changes = false;
    }
    if (ev.key == keys::S && ev.with(mods::CTRL) && ev.with(mods::SHIFT)) {
        state_.ui.need_save_as_modal = true;
    }
}

inline void app::handle_mouse_move(
    const gfx::mouse_move_event& ev
) {
    const auto& io = ImGui::GetIO();
    if (io.WantCaptureMouse) {
        return;
    }

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

inline void app::handle_animation_actions_() {
    if (state_.need_toggle_playback) {
        state_.need_toggle_playback = false;
        handle_toggle_playback();
    }

    if (state_.need_stop_playback) {
        state_.need_stop_playback = false;
        handle_stop_playback();
    }

    if (state_.need_add_keyframe) {
        state_.need_add_keyframe = false;
        handle_add_keyframe();
    }

    if (state_.need_delete_keyframe) {
        state_.need_delete_keyframe = false;
        handle_delete_keyframe();
    }
}

inline void app::handle_toggle_playback() {
    if (state_.selected_clip_name.empty() || state_.root_name.empty() ||
        !state_.name_to_entity.contains(state_.root_name)) {
        return;
    }

    auto& world   = get_engine().get_world();
    auto root_ent = state_.name_to_entity[state_.root_name];

    if (state_.is_previewing) {
        if (world.has_component<gfx::animation_player_component>(root_ent)) {
            world.get_animation_system().modify_player(root_ent).layer(0).pause();
        }
        state_.is_previewing = false;
        return;
    }

    auto& clip_reg = world.get_animation_clip_registry();
    auto clip      = clip_reg.get(state_.selected_clip_name);
    if (!clip) {
        return;
    }

    if (!world.has_component<gfx::animation_player_component>(root_ent)) {
        if (auto* guard = state_.find_guard(root_ent)) {
            guard->with<gfx::animation_player_component>();
        }
    }

    auto& anim_sys = world.get_animation_system();
    auto& player   = world.get_component<gfx::animation_player_component>(root_ent);

    if (player.has_layer(0) && player.get_layer(0).state == gfx::animation_state::paused) {
        anim_sys.modify_player(root_ent).layer(0).resume();
    } else {
        anim_sys.modify_player(root_ent).layer(0).blend_to(clip);
        anim_sys.modify_player(root_ent).layer(0).play();
    }
    state_.is_previewing = true;
}

inline void app::handle_stop_playback() {
    if (!state_.root_name.empty() && state_.name_to_entity.contains(state_.root_name)) {
        auto& world   = get_engine().get_world();
        auto root_ent = state_.name_to_entity[state_.root_name];
        if (world.has_component<gfx::animation_player_component>(root_ent)) {
            world.get_animation_system().modify_player(root_ent).layer(0).stop();
        }
    }
    state_.is_previewing   = false;
    state_.timeline_cursor = 0.f;
}

inline void app::handle_add_keyframe() {
    if (state_.selected_clip_name.empty() || state_.selected_name.empty()) {
        return;
    }

    auto& world    = get_engine().get_world();
    auto& clip_reg = world.get_animation_clip_registry();
    auto clip      = clip_reg.get(state_.selected_clip_name);
    if (!clip) {
        return;
    }

    auto entity_name = state_.selected_name;
    auto ent         = state_.name_to_entity[entity_name];
    auto prop        = state_.selected_property;
    float32 time     = state_.timeline_cursor;

    if (!world.has_component<gfx::transform_component>(ent)) {
        return;
    }

    auto& tc = world.get_component<gfx::transform_component>(ent);

    keyframe_value kf_val;
    if (prop == gfx::animation_property::rotation) {
        kf_val = gfx::keyframe_quat{time, math::euler_to_quat(tc.get_rotation())};
    } else if (prop == gfx::animation_property::position) {
        kf_val = gfx::keyframe_vec3f{time, tc.get_position()};
    } else if (prop == gfx::animation_property::scale) {
        kf_val = gfx::keyframe_vec3f{time, tc.get_scale()};
    } else {
        kf_val = gfx::keyframe_vec3f{time, tc.get_origin()};
    }

    if (!clip->has_track(entity_name)) {
        add_track_params params = {
            .clip_name  = state_.selected_clip_name,
            .track_name = entity_name,
            .property   = prop,
            .keyframe   = kf_val,
        };
        auto op = std::make_unique<add_track_operation>(get_engine(), state_, params);
        op_manager_.execute(std::move(op));
    } else {
        add_keyframe_params params = {
            .clip_name  = state_.selected_clip_name,
            .track_name = entity_name,
            .property   = prop,
            .keyframe   = kf_val,
        };
        auto op = std::make_unique<add_keyframe_operation>(get_engine(), state_, params);
        op_manager_.execute(std::move(op));
    }
}

inline void app::handle_delete_keyframe() {
    if (state_.selected_keyframe_time < 0.f || state_.selected_clip_name.empty() ||
        state_.selected_track_name.empty()) {
        return;
    }

    auto& world          = get_engine().get_world();
    const auto& clip_reg = world.get_animation_clip_registry();
    const auto clip      = clip_reg.get(state_.selected_clip_name);
    if (!clip) {
        return;
    }

    const auto* track = clip->get_track(state_.selected_track_name);
    if (track == nullptr) {
        return;
    }

    const auto* ch = track->get_channel(state_.selected_property);
    if (ch == nullptr) {
        return;
    }

    std::visit(
        [&](const auto& channel) {
            for (const auto& kf : channel.get_keyframes()) {
                if (std::abs(kf.time - state_.selected_keyframe_time) < 0.0001f) {
                    remove_keyframe_params params;
                    params.clip_name  = state_.selected_clip_name;
                    params.track_name = state_.selected_track_name;
                    params.property   = state_.selected_property;
                    params.keyframe   = keyframe_value(kf);
                    auto op =
                        std::make_unique<remove_keyframe_operation>(get_engine(), state_, params);
                    op_manager_.execute(std::move(op));
                    return;
                }
            }
        },
        *ch
    );
}

inline void app::update_title_() {
    std::string title;
    if (state_.filename.empty()) {
        title = std::format("Sculptor {}", version_string);
    } else {
        auto filename_suffix = state_.has_unsaved_changes ? "*" : "";
        title = std::format("Sculptor {} | {}{}", version_string, state_.filename, filename_suffix);

        if (!state_.selected_clip_name.empty()) {
            auto clip_suffix = state_.has_unsaved_clip(state_.selected_clip_name) ? "*" : "";
            title += std::format(" | {}.voxa{}", state_.selected_clip_name, clip_suffix);
        }

        if (state_.has_unsaved_changes) {
            title += " | UNSAVED CHANGES";
        }
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
