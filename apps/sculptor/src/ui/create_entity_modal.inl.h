#pragma once

#ifndef VW_SCULPTOR_CREATE_ENTITY_MODAL_INL_H
#define VW_SCULPTOR_CREATE_ENTITY_MODAL_INL_H
#include <imgui.h>
#include "operations/create_entity_operation.h"

namespace vw::sculptor {

inline create_entity_modal::create_entity_modal(
    engine_type& eng, app_state& state, operation_manager& op_manager
)
    : engine_(&eng), state_(&state), op_manager_(&op_manager) {}

inline void create_entity_modal::open() {
    need_open_ = true;
    name_        = std::format("new entity {}", state_->scene.name_to_entity.size());
    with_model_  = false;
    with_socket_ = false;
}

inline void create_entity_modal::render(
    float /*delta_time*/
) {
    if (need_open_) {
        ImGui::OpenPopup("Create Entity");
        need_open_ = false;
    }

    ImGuiWindowFlags dialog_flags =          //
        ImGuiWindowFlags_AlwaysAutoResize |  //
        ImGuiWindowFlags_NoMove;
    if (ImGui::BeginPopupModal("Create Entity", nullptr, dialog_flags)) {
        if (!error_.empty()) {
            ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "%s", error_.c_str());
        }

        imgui_input_text_string("Name", name_);

        ImGui::Checkbox("With Model", &with_model_);
        if (with_model_) {
            imgui_input_int_left("Size X", &size_.x);
            imgui_input_int_left("Size Y", &size_.y);
            imgui_input_int_left("Size Z", &size_.z);
        }

        ImGui::Checkbox("With Socket", &with_socket_);

        if (ImGui::Button("Create")) {
            if (create_entity()) {
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

inline auto create_entity_modal::create_entity() -> bool {
    error_.clear();

    if (name_.empty()) {
        error_ = "Name cannot be empty.";
        return false;
    }
    if (size_.x <= 0 || size_.y <= 0 || size_.z <= 0) {
        error_ = "Size dimensions must be greater than zero.";
        return false;
    }
    if (state_->scene.name_to_entity.contains(name_)) {
        error_ = "An entity with this name already exists.";
        return false;
    }

    create_entity_params params = {
        .name        = name_,
        .parent_name = state_->scene.selected_name,
        .with_model  = with_model_,
        .with_socket = with_socket_,
        .size        = size_,
    };

    auto op = std::make_unique<create_entity_operation>(*engine_, *state_, params);
    op_manager_->execute(std::move(op));

    return true;
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_CREATE_ENTITY_MODAL_INL_H
