#pragma once

#ifndef VW_SCULPTOR_CREATE_ENTITY_MODAL_INL_H
#define VW_SCULPTOR_CREATE_ENTITY_MODAL_INL_H

namespace vw::sculptor {

inline create_entity_modal::create_entity_modal(
    engine_type& eng, app_state& state, operation_manager& op_manager
)
    : engine_(&eng), state_(&state), op_manager_(&op_manager) {}

inline void create_entity_modal::open() {
    need_open_ = true;
    name_      = std::format("new entity {}", state_->name_to_entity.size());
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

        ImGui::InputInt("Size X", &size_.x);
        ImGui::InputInt("Size Y", &size_.y);
        ImGui::InputInt("Size Z", &size_.z);

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

inline bool create_entity_modal::create_entity() {
    error_.clear();

    if (name_.empty()) {
        error_ = "Name cannot be empty.";
        return false;
    }
    if (size_.x <= 0 || size_.y <= 0 || size_.z <= 0) {
        error_ = "Size dimensions must be greater than zero.";
        return false;
    }
    if (state_->name_to_entity.contains(name_)) {
        error_ = "An entity with this name already exists.";
        return false;
    }

    create_entity_params params = {
        .name        = name_,
        .parent_name = state_->selected_name,
        .with_model  = true,
        .size        = size_,
    };

    auto op = std::make_unique<create_entity_operation>(*engine_, *state_, params);
    op_manager_->execute(std::move(op));

    return true;
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_CREATE_ENTITY_MODAL_INL_H
