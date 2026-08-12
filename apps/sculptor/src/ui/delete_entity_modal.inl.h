#pragma once

#ifndef VW_SCULPTOR_DELETE_ENTITY_MODAL_INL_H
#define VW_SCULPTOR_DELETE_ENTITY_MODAL_INL_H
#include <imgui.h>


namespace vw::sculptor {

inline delete_entity_modal::delete_entity_modal(
    engine_type& eng, app_state& state, operation_manager& op_manager
)
    : engine_(&eng), state_(&state), op_manager_(&op_manager) {}

inline void delete_entity_modal::open(
    const std::string& delete_name
) {
    need_open_   = true;
    delete_name_ = delete_name;
}

inline void delete_entity_modal::render(
    float delta_time
) {
    if (need_open_) {
        ImGui::OpenPopup("Delete Entity");
        need_open_ = false;
    }

    ImGuiWindowFlags dialog_flags =          //
        ImGuiWindowFlags_AlwaysAutoResize |  //
        ImGuiWindowFlags_NoMove;
    if (ImGui::BeginPopupModal("Delete Entity", nullptr, dialog_flags)) {
        ImGui::Text("Are you sure want to delete \"%s\"?", delete_name_.c_str());

        if (ImGui::Button("Delete")) {
            delete_entity_params params = {
                .name = delete_name_
            };

            auto op = std::make_unique<delete_entity_operation>(*engine_, *state_, params);
            op_manager_->execute(std::move(op));

            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_DELETE_ENTITY_MODAL_INL_H
