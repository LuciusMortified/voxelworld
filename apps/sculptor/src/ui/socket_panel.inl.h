#pragma once

#ifndef VW_SCULPTOR_SOCKET_PANEL_INL_H
#define VW_SCULPTOR_SOCKET_PANEL_INL_H

#include <filesystem>

#include "vw/gfx/world/serializers/vox_deserializer.h"

namespace vw::sculptor {

inline socket_panel::socket_panel(
    engine_type& eng, app_state& st, operation_manager& op_manager
)
    : engine_(&eng), state_(&st), op_manager_(&op_manager) {}

inline void socket_panel::render(
    float /*delta_time*/
) {
    if (state_->selected_name.empty()) {
        return;
    }

    auto& world    = engine_->get_world();
    const auto ent = state_->name_to_entity[state_->selected_name];

    if (!world.has_component<gfx::socket_component>(ent)) {
        return;
    }

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 window_pos       = ImVec2(
        viewport->WorkPos.x + viewport->WorkSize.x - 10,
        viewport->WorkPos.y + state_->ui.right_top_voffset + 10
    );
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(1.0f, 0.0f));

    constexpr ImGuiWindowFlags window_flags =  //
        ImGuiWindowFlags_NoSavedSettings |     //
        ImGuiWindowFlags_NoMove |              //
        ImGuiWindowFlags_AlwaysAutoResize;

    ImGui::Begin("Sockets", nullptr, window_flags);

    const auto& socket_comp = world.template get_component<gfx::socket_component>(ent);
    const auto& sockets     = socket_comp.get_sockets();

    if (sockets.empty()) {
        ImGui::TextDisabled("No sockets");
    } else {
        std::string socket_to_remove;
        for (const auto& sp : sockets) {
            ImGui::PushID(sp.name.c_str());
            render_socket_(ent, sp);

            ImGui::SameLine();
            if (ImGui::SmallButton("X")) {
                socket_to_remove = sp.name;
            }
            ImGui::PopID();
        }

        if (!socket_to_remove.empty()) {
            remove_socket_params params = {
                .entity_name = state_->selected_name,
                .socket_name = socket_to_remove,
            };
            auto op = std::make_unique<remove_socket_operation>(*engine_, *state_, params);
            op_manager_->execute(std::move(op));

            unload_preview_(socket_to_remove);
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    render_add_socket_(ent);

    ImGui::Dummy({220.0f, 0.0f});

    state_->ui.right_top_voffset += ImGui::GetWindowHeight() + 10.0f;

    ImGui::End();

    render_preview_file_list_();
}

inline void socket_panel::render_socket_(
    gfx::entity ent, const gfx::socket_point& sp
) {
    if (ImGui::TreeNode(sp.name.c_str())) {
        vec3f position     = sp.position;
        vec3f rotation_deg = {
            math::degrees(sp.rotation.x),
            math::degrees(sp.rotation.y),
            math::degrees(sp.rotation.z),
        };
        vec3f scale = sp.scale;

        bool changed = false;

        ImGui::PushItemWidth(60.0f);
        ImGui::Text("Pos");
        ImGui::SameLine(40.f);
        changed |= ImGui::DragFloat("##PX", &position.x, 0.1f, 0, 0, "%.2f");
        ImGui::SameLine();
        changed |= ImGui::DragFloat("##PY", &position.y, 0.1f, 0, 0, "%.2f");
        ImGui::SameLine();
        changed |= ImGui::DragFloat("##PZ", &position.z, 0.1f, 0, 0, "%.2f");

        ImGui::Text("Rot");
        ImGui::SameLine(40.f);
        changed |= ImGui::DragFloat("##RX", &rotation_deg.x, 0.5f, 0, 0, "%.1f");
        ImGui::SameLine();
        changed |= ImGui::DragFloat("##RY", &rotation_deg.y, 0.5f, 0, 0, "%.1f");
        ImGui::SameLine();
        changed |= ImGui::DragFloat("##RZ", &rotation_deg.z, 0.5f, 0, 0, "%.1f");

        ImGui::Text("Scale");
        ImGui::SameLine(40.f);
        changed |= ImGui::DragFloat("##SX", &scale.x, 0.01f, 0, 0, "%.2f");
        ImGui::SameLine();
        changed |= ImGui::DragFloat("##SY", &scale.y, 0.01f, 0, 0, "%.2f");
        ImGui::SameLine();
        changed |= ImGui::DragFloat("##SZ", &scale.z, 0.01f, 0, 0, "%.2f");
        ImGui::PopItemWidth();

        if (changed) {
            vec3f new_rotation = {
                math::radians(rotation_deg.x),
                math::radians(rotation_deg.y),
                math::radians(rotation_deg.z),
            };

            set_socket_transform_params params = {
                .entity_name = state_->selected_name,
                .socket_name = sp.name,
                .position    = position,
                .rotation    = new_rotation,
                .scale       = scale,
            };
            auto op = std::make_unique<set_socket_transform_operation>(*engine_, *state_, params);
            op_manager_->execute(std::move(op));

            update_preview_transform_(sp.name, position, new_rotation, scale);
        }

        if (sp.attached.is_valid()) {
            auto it = state_->entity_to_name.find(sp.attached);
            if (it != state_->entity_to_name.end()) {
                ImGui::Text("Attached: %s", it->second.c_str());
            } else {
                ImGui::Text("Attached: %u.%u", sp.attached.index, sp.attached.generation);
            }
        } else {
            ImGui::TextDisabled("Empty");
        }

        bool has_preview = state_->socket_previews.contains(sp.name);

        if (has_preview) {
            auto& preview = state_->socket_previews[sp.name];
            ImGui::Text("Preview: %s", preview.filename.c_str());
            ImGui::SameLine();
            if (ImGui::SmallButton("Unload")) {
                unload_preview_(sp.name);
            }
        } else {
            if (ImGui::SmallButton("Load Preview")) {
                need_preview_modal_ = true;
                preview_modal_key_  = sp.name;
            }
        }

        ImGui::TreePop();
    }
}

inline void socket_panel::render_add_socket_(
    gfx::entity /*ent*/
) {
    ImGui::PushItemWidth(120.0f);
    imgui_input_text_string("##new_socket", new_socket_name_);
    ImGui::PopItemWidth();
    ImGui::SameLine();

    bool disabled = new_socket_name_.empty();
    if (disabled) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Add Socket")) {
        add_socket_params params = {
            .entity_name = state_->selected_name,
            .socket_name = new_socket_name_,
        };
        auto op = std::make_unique<add_socket_operation>(*engine_, *state_, params);
        op_manager_->execute(std::move(op));
        new_socket_name_.clear();
    }
    if (disabled) {
        ImGui::EndDisabled();
    }
}

inline void socket_panel::render_preview_file_list_() {
    if (need_preview_modal_) {
        ImGui::OpenPopup("Load Preview");
        need_preview_modal_ = false;

        namespace fs = std::filesystem;
        vox_filenames_.clear();
        fs::path asset_dir{app_state::asset_dir_name};
        if (fs::exists(asset_dir)) {
            for (const auto& entry : fs::directory_iterator(asset_dir)) {
                if (entry.is_regular_file() && entry.path().extension() == ".vox") {
                    vox_filenames_.emplace_back(entry.path().filename().string());
                }
            }
        }
    }

    ImGuiWindowFlags dialog_flags =          //
        ImGuiWindowFlags_AlwaysAutoResize |  //
        ImGuiWindowFlags_NoMove;
    if (ImGui::BeginPopupModal("Load Preview", nullptr, dialog_flags)) {
        ImGui::Text("Select VOX file:");
        ImGui::Spacing();

        const float list_height     = ImGui::GetTextLineHeightWithSpacing() * 7.5f;
        ImGuiChildFlags child_flags =                 //
            ImGuiChildFlags_AlwaysUseWindowPadding |  //
            ImGuiChildFlags_Borders;
        static std::string selected_file;
        if (ImGui::BeginChild("##preview_file_list", ImVec2(300.f, list_height), child_flags)) {
            for (const auto& f : vox_filenames_) {
                bool is_selected = selected_file == f;
                if (ImGui::Selectable(f.c_str(), is_selected)) {
                    selected_file = f;
                }
            }
            ImGui::EndChild();
        }

        ImGui::Spacing();

        if (selected_file.empty()) {
            ImGui::BeginDisabled();
        }
        if (ImGui::Button("Load")) {
            load_preview_(preview_modal_key_, selected_file);
            selected_file.clear();
            ImGui::CloseCurrentPopup();
        }
        if (selected_file.empty()) {
            ImGui::EndDisabled();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            selected_file.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

inline void socket_panel::load_preview_(
    const std::string& socket_name, const std::string& filename
) {
    unload_preview_(socket_name);

    namespace fs = std::filesystem;

    gfx::vox_deserializer deserializer{engine_->get_world()};
    fs::path filepath = fs::path{app_state::asset_dir_name} / fs::path{filename};

    gfx::vox_deserializer<>::options opts;
    opts.skip_sockets = true;
    opts.skip_targets = true;
    auto result = deserializer.deserialize(filepath, opts);
    if (!result.has_value()) {
        return;
    }

    auto parent_ent   = state_->name_to_entity[state_->selected_name];
    auto& world       = engine_->get_world();
    auto& socket_comp = world.template get_component<gfx::socket_component>(parent_ent);
    const auto* sp    = socket_comp.find(socket_name);
    if (!sp) {
        return;
    }

    app_state::socket_preview preview;
    preview.filename          = filename;
    preview.preview_root_name = result->root_name;
    preview.guards            = std::move(result->entities);

    if (result->name_to_entity.contains(result->root_name)) {
        auto preview_root      = result->name_to_entity[result->root_name];
        auto& transform_system = world.get_transform_system();
        transform_system.modify(preview_root)
            .set_position(sp->position)
            .set_rotation(sp->rotation)
            .set_scale(sp->scale);

        auto& hierarchy_system = world.get_hierarchy_system();
        hierarchy_system.modify(preview_root).set_parent(parent_ent);
    }

    state_->socket_previews[socket_name] = std::move(preview);
}

inline void socket_panel::unload_preview_(
    const std::string& key
) {
    state_->socket_previews.erase(key);
}

inline void socket_panel::update_preview_transform_(
    const std::string& key, const vec3f& position, const vec3f& rotation, const vec3f& scale
) {
    auto it = state_->socket_previews.find(key);
    if (it == state_->socket_previews.end()) {
        return;
    }

    auto& preview = it->second;
    if (preview.guards.empty()) {
        return;
    }

    auto preview_root      = preview.guards[0]->get_entity();
    auto& transform_system = engine_->get_world().get_transform_system();
    transform_system.modify(preview_root)
        .set_position(position)
        .set_rotation(rotation)
        .set_scale(scale);
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_SOCKET_PANEL_INL_H
