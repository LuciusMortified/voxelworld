#pragma once

#ifndef VW_SCULPTOR_CLIP_MANAGER_PANEL_INL_H
#define VW_SCULPTOR_CLIP_MANAGER_PANEL_INL_H

#include <filesystem>

#include "operations/close_clip_operation.h"
#include "vw/gfx/world/serializers/voxa_deserializer.h"
#include "vw/gfx/world/serializers/voxa_serializer.h"

namespace vw::sculptor {

inline clip_manager_panel::clip_manager_panel(
    engine_type& eng, app_state& st, operation_manager& op_manager
)
    : engine_(&eng)
    , state_(&st)
    , op_manager_(&op_manager)
    , create_modal_(eng, st, op_manager)
    , layer_blend_modal_(st) {}

inline void clip_manager_panel::render(
    float delta_time
) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const auto window_pos         = ImVec2(
        viewport->WorkPos.x + viewport->WorkSize.x - 10,
        viewport->WorkPos.y + state_->ui.right_top_voffset + 10
    );
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(1.0f, 0.0f));

    constexpr ImGuiWindowFlags window_flags =  //
        ImGuiWindowFlags_NoSavedSettings |     //
        ImGuiWindowFlags_AlwaysAutoResize |    //
        ImGuiWindowFlags_NoMove;

    bool still_open = true;
    ImGui::Begin("Animation Clips", &still_open, window_flags);
    if (!still_open) {
        if (state_->animation_mode) {
            exit_animation_mode_();
        }
        state_->ui.show_clip_manager = false;
        state_->ui.show_timeline     = false;
    } else if (!state_->animation_mode) {
        enter_animation_mode_();
    }

    if (state_->ui.need_create_clip_modal) {
        create_modal_.open();
        state_->ui.need_create_clip_modal = false;
    }

    if (state_->ui.need_save_clip) {
        state_->ui.need_save_clip = false;
        save_clip_();
    }

    if (state_->ui.need_load_clip_modal) {
        load_voxa_filenames_();
        selected_load_filename_.clear();
        ImGui::OpenPopup("Open Animation");
        state_->ui.need_load_clip_modal = false;
    }

    if (state_->ui.need_close_clip) {
        state_->ui.need_close_clip = false;
        if (state_->has_unsaved_clip(state_->selected_clip_name)) {
            need_close_confirm_popup_ = true;
        } else {
            close_clip_();
        }
    }

    const bool has_selected = !state_->selected_clip_name.empty() &&
        engine_->get_world().get_animation_clip_registry().has(state_->selected_clip_name);

    if (ImGui::Button("New")) {
        create_modal_.open();
    }

    ImGui::SameLine();
    if (ImGui::Button("Open")) {
        load_voxa_filenames_();
        selected_load_filename_.clear();
        need_load_popup_ = true;
    }

    ImGui::SameLine();
    ImGui::BeginDisabled(!has_selected);
    if (ImGui::Button("Save")) {
        save_clip_();
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::BeginDisabled(!has_selected);
    if (ImGui::Button("Close")) {
        if (state_->has_unsaved_clip(state_->selected_clip_name)) {
            need_close_confirm_popup_ = true;
        } else {
            close_clip_();
        }
    }
    ImGui::EndDisabled();

    if (state_->animation_mode) {
        ImGui::SameLine();
        if (ImGui::Button("Reset All")) {
            reset_all_();
        }
    }

    if (need_close_confirm_popup_) {
        ImGui::OpenPopup("Close Animation?");
        need_close_confirm_popup_ = false;
    }

    render_close_confirm_popup_();

    if (need_load_popup_) {
        ImGui::OpenPopup("Open Animation");
        need_load_popup_ = false;
    }
    render_load_popup_();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const float list_height = ImGui::GetTextLineHeightWithSpacing() * 7.5f;

    constexpr ImGuiChildFlags child_flags =       //
        ImGuiChildFlags_AlwaysUseWindowPadding |  //
        ImGuiChildFlags_Borders;

    if (ImGui::BeginChild("##clip_list", ImVec2(0.f, list_height), child_flags)) {
        const auto& registry = engine_->get_world().get_animation_clip_registry();
        for (const auto& [name, clip] : registry.all()) {
            const bool is_selected = (state_->selected_clip_name == name);
            const bool is_unsaved  = state_->has_unsaved_clip(name);

            if (clip) {
                ImGui::TextDisabled("[%zu]", state_->get_layer_for_clip(name));
                ImGui::SameLine();
            }

            if (is_unsaved) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.9f, 0.4f, 1.f));
            }

            auto display_name = is_unsaved ? std::format("{}*", name) : name;
            if (ImGui::Selectable(display_name.c_str(), is_selected, false)) {
                if (!is_selected) {
                    state_->selected_clip_name = name;
                    state_->ui.show_timeline   = true;
                    state_->selected_track_name.clear();
                    state_->selected_keyframe_id = gfx::invalid_keyframe_id;
                }
            }

            if (is_unsaved) {
                ImGui::PopStyleColor();
            }
        }
        ImGui::EndChild();
    }

    if (has_selected) {
        ImGui::Spacing();
        ImGui::Text("Layer");
        ImGui::SameLine();
        const auto current_layer = state_->get_layer_for_clip(state_->selected_clip_name);
        for (int i = 0; i < 5; ++i) {
            if (i > 0) {
                ImGui::SameLine();
            }
            bool is_active = (current_layer == static_cast<size_t>(i));
            if (is_active) {
                ImGui::PushStyleColor(
                    ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive)
                );
            }
            auto btn_id = std::format("{}##layer_btn", i);
            if (ImGui::SmallButton(btn_id.c_str())) {
                stop_layer_for_clip_(state_->selected_clip_name);
                state_->clip_to_layer[state_->selected_clip_name] = static_cast<size_t>(i);
            }
            if (is_active) {
                ImGui::PopStyleColor();
            }
        }

        const auto layer_idx = state_->get_layer_for_clip(state_->selected_clip_name);
        if (layer_idx > 0) {
            ImGui::SameLine();
            if (ImGui::SmallButton("Blend")) {
                layer_blend_modal_.open();
            }
        }
    }

    create_modal_.render(delta_time);
    layer_blend_modal_.render(delta_time);

    ImGui::Dummy({200.0f, 0.0f});

    state_->ui.right_top_voffset += ImGui::GetWindowHeight() + 10.0f;

    ImGui::End();
}

inline void clip_manager_panel::render_close_confirm_popup_() const {
    if (ImGui::BeginPopupModal("Close Animation?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text(
            "Animation \"%s\" has unsaved changes. Close anyway?",
            state_->selected_clip_name.c_str()
        );
        ImGui::Spacing();

        if (ImGui::Button("Save & Close")) {
            save_clip_();
            close_clip_();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Close")) {
            close_clip_();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

inline void clip_manager_panel::close_clip_() const {
    close_clip_params params = {.name = state_->selected_clip_name};
    auto op                  = std::make_unique<close_clip_operation>(*engine_, *state_, params);
    op_manager_->execute(std::move(op));
}

inline void clip_manager_panel::render_load_popup_() {
    ImGuiWindowFlags dialog_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove;

    if (ImGui::BeginPopupModal("Open Animation", nullptr, dialog_flags)) {
        ImGui::Text("Animation files:");
        ImGui::Spacing();

        const float list_height = ImGui::GetTextLineHeightWithSpacing() * 7.5f;
        ImGuiChildFlags child_flags =
            ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_Borders;
        if (ImGui::BeginChild("##voxa_file_list", ImVec2(400.f, list_height), child_flags)) {
            if (voxa_filenames_.empty()) {
                ImGui::TextDisabled("No .voxa files found");
            } else {
                for (const auto& filename : voxa_filenames_) {
                    bool is_selected = selected_load_filename_ == filename;
                    if (ImGui::Selectable(filename.c_str(), is_selected)) {
                        selected_load_filename_ = filename;
                    }
                }
            }
            ImGui::EndChild();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (selected_load_filename_.empty()) {
            ImGui::BeginDisabled();
        }
        if (ImGui::Button("Open")) {
            if (load_clip_(selected_load_filename_)) {
                ImGui::CloseCurrentPopup();
            }
        }
        if (selected_load_filename_.empty()) {
            ImGui::EndDisabled();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

inline void clip_manager_panel::load_voxa_filenames_() {
    namespace fs = std::filesystem;

    voxa_filenames_.clear();

    fs::path asset_dir_path{app_state::asset_dir_name};
    if (!fs::exists(asset_dir_path)) {
        return;
    }

    for (const auto& entry : fs::directory_iterator(asset_dir_path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".voxa") {
            voxa_filenames_.emplace_back(entry.path().filename().string());
        }
    }
}

inline auto clip_manager_panel::save_clip_() const -> bool {
    namespace fs = std::filesystem;

    const auto& registry = engine_->get_world().get_animation_clip_registry();
    const auto clip      = registry.get(state_->selected_clip_name);
    if (!clip) {
        return false;
    }

    const fs::path asset_dir_path{app_state::asset_dir_name};
    const fs::path filepath = asset_dir_path / std::format("{}.voxa", clip->get_name());
    gfx::voxa_serializer serializer(*clip);
    const auto result = serializer.serialize(filepath);
    if (result.has_value()) {
        state_->unsaved_clips[state_->selected_clip_name] = false;
    }
    return result.has_value();
}

inline void clip_manager_panel::save_all_clips() const {
    namespace fs = std::filesystem;

    const auto& registry = engine_->get_world().get_animation_clip_registry();
    const fs::path asset_dir_path{app_state::asset_dir_name};

    for (const auto& [name, clip] : registry.all()) {
        if (!clip) {
            continue;
        }

        fs::path filepath = asset_dir_path / std::format("{}.voxa", name);
        gfx::voxa_serializer serializer(*clip);
        if (serializer.serialize(filepath).has_value()) {
            state_->unsaved_clips[name] = false;
        }
    }
}

inline auto clip_manager_panel::load_clip_(
    const std::string& filename
) const -> bool {
    namespace fs = std::filesystem;

    const fs::path filepath = fs::path{app_state::asset_dir_name} / fs::path{filename};
    gfx::voxa_deserializer deserializer;
    const auto result = deserializer.deserialize(filepath);
    if (!result) {
        return false;
    }

    const auto& clip = *result;
    auto& registry   = engine_->get_world().get_animation_clip_registry();
    registry.add(clip->get_name(), clip);

    state_->selected_clip_name = clip->get_name();
    state_->ui.show_timeline   = true;
    state_->selected_track_name.clear();
    state_->selected_keyframe_id = gfx::invalid_keyframe_id;

    return true;
}

inline void clip_manager_panel::stop_layer_for_clip_(
    const std::string& clip_name
) const {
    if (state_->root_name.empty() || !state_->name_to_entity.contains(state_->root_name)) {
        return;
    }

    auto root   = state_->name_to_entity[state_->root_name];
    auto& world = engine_->get_world();
    if (!world.has_component<gfx::animation_player_component>(root)) {
        return;
    }

    const auto layer_idx = state_->get_layer_for_clip(clip_name);
    const auto& player   = world.get_component<gfx::animation_player_component>(root);
    if (player.has_layer(layer_idx) && player.get_layer(layer_idx).clip &&
        player.get_layer(layer_idx).clip->get_name() == clip_name) {
        world.get_animation_system().modify_player(root).layer(layer_idx).stop();
    }
}

inline void clip_manager_panel::force_exit_animation_mode() const {
    if (state_->animation_mode) {
        exit_animation_mode_();
    }
    state_->ui.show_timeline = false;
}

inline void clip_manager_panel::stop_all_layers_() const {
    if (state_->root_name.empty() || !state_->name_to_entity.contains(state_->root_name)) {
        return;
    }

    const auto root = state_->name_to_entity[state_->root_name];
    auto& world     = engine_->get_world();
    if (!world.has_component<gfx::animation_player_component>(root)) {
        return;
    }

    const auto& player = world.get_component<gfx::animation_player_component>(root);
    auto& anim_sys     = world.get_animation_system();
    for (size_t i = 0; i < player.layer_count(); ++i) {
        if (player.has_layer(i)) {
            anim_sys.modify_player(root).layer(i).clear();
        }
    }
}

inline void clip_manager_panel::enter_animation_mode_() const {
    if (state_->animation_mode) {
        return;
    }

    if (!state_->has_saved_transforms) {
        auto& world = engine_->get_world();
        state_->saved_transforms.clear();
        for (const auto& [name, ent] : state_->name_to_entity) {
            if (world.has_component<gfx::transform_component>(ent)) {
                auto& tc                       = world.get_component<gfx::transform_component>(ent);
                state_->saved_transforms[name] = tc.get_transform();
            }
        }
        state_->has_saved_transforms = true;
    }

    state_->animation_mode = true;
}

inline void clip_manager_panel::exit_animation_mode_() const {
    stop_all_layers_();
    restore_transforms_();
    state_->animation_mode  = false;
    state_->timeline_cursor = 0.f;
}

inline void clip_manager_panel::reset_all_() const {
    stop_all_layers_();

    if (state_->has_saved_transforms) {
        auto& world            = engine_->get_world();
        auto& transform_system = world.get_transform_system();
        for (const auto& [name, t] : state_->saved_transforms) {
            if (state_->name_to_entity.contains(name)) {
                const auto ent = state_->name_to_entity[name];
                if (world.has_component<gfx::transform_component>(ent)) {
                    transform_system.modify(ent).set_transform(t);
                }
            }
        }
    }

    state_->timeline_cursor = 0.f;
}

inline void clip_manager_panel::restore_transforms_() const {
    if (!state_->has_saved_transforms) {
        return;
    }

    auto& world            = engine_->get_world();
    auto& transform_system = world.get_transform_system();

    for (const auto& [name, t] : state_->saved_transforms) {
        if (state_->name_to_entity.contains(name)) {
            const auto ent = state_->name_to_entity[name];
            if (world.has_component<gfx::transform_component>(ent)) {
                transform_system.modify(ent).set_transform(t);
            }
        }
    }

    state_->saved_transforms.clear();
    state_->has_saved_transforms = false;
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_CLIP_MANAGER_PANEL_INL_H
