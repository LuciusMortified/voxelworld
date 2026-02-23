#pragma once

#ifndef VW_SCULPTOR_LAYER_BLEND_MODAL_INL_H
#define VW_SCULPTOR_LAYER_BLEND_MODAL_INL_H

namespace vw::sculptor {

inline layer_blend_modal::layer_blend_modal(
    app_state& st
)
    : state_(&st) {}

inline void layer_blend_modal::open() {
    need_open_ = true;

    const auto& cs     = state_->get_clip_settings(state_->selected_clip_name);
    fade_in_duration_  = cs.fade_in.duration;
    fade_in_interp_    = static_cast<int>(cs.fade_in.interp);
    fade_out_duration_ = cs.fade_out.duration;
    fade_out_interp_   = static_cast<int>(cs.fade_out.interp);
}

inline void layer_blend_modal::render(
    float /*delta_time*/
) {
    if (need_open_) {
        ImGui::OpenPopup("Layer Blend");
        need_open_ = false;
    }

    constexpr ImGuiWindowFlags dialog_flags =  //
        ImGuiWindowFlags_AlwaysAutoResize |    //
        ImGuiWindowFlags_NoMove;

    if (ImGui::BeginPopupModal("Layer Blend", nullptr, dialog_flags)) {
        constexpr std::array interp_names = {
            "Linear", "Step", "Ease In", "Ease Out", "Ease In/Out", "Cubic Bezier"
        };

        ImGui::Text("Fade In");
        ImGui::PushItemWidth(100.f);
        ImGui::DragFloat("Duration##fade_in", &fade_in_duration_, 0.01f, 0.f, 10.f, "%.2fs");
        ImGui::SameLine();
        ImGui::Combo("Interp##fade_in", &fade_in_interp_, interp_names.data(), interp_names.size());
        ImGui::PopItemWidth();

        ImGui::Spacing();

        ImGui::Text("Fade Out");
        ImGui::PushItemWidth(100.f);
        ImGui::DragFloat("Duration##fade_out", &fade_out_duration_, 0.01f, 0.f, 10.f, "%.2fs");
        ImGui::SameLine();
        ImGui::Combo(
            "Interp##fade_out", &fade_out_interp_, interp_names.data(), interp_names.size()
        );
        ImGui::PopItemWidth();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Apply")) {
            auto& cs         = state_->get_clip_settings_mut(state_->selected_clip_name);
            cs.fade_in.duration  = fade_in_duration_;
            cs.fade_in.interp    = static_cast<math::interpolation_type>(fade_in_interp_);
            cs.fade_out.duration = fade_out_duration_;
            cs.fade_out.interp   = static_cast<math::interpolation_type>(fade_out_interp_);
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

#endif  // VW_SCULPTOR_LAYER_BLEND_MODAL_INL_H
