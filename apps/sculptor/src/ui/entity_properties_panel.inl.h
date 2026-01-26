#pragma once

#ifndef VW_SCULPTOR_ENTITY_PROPERTIES_PANEL_INL_H
#define VW_SCULPTOR_ENTITY_PROPERTIES_PANEL_INL_H

namespace vw::sculptor {

inline entity_properties_panel::entity_properties_panel(
    engine_type& eng, app_state& st, operation_manager& op_manager
)
    : engine_(&eng), state_(&st), op_manager_(&op_manager) {}

inline void entity_properties_panel::render(
    float /*delta_time*/
) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 window_pos       = ImVec2(
        viewport->WorkPos.x + viewport->WorkSize.x - 10,
        viewport->WorkPos.y + state_->ui.right_side_voffset + 10
    );
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(1.0f, 0.0f));

    ImGuiWindowFlags window_flags =         //
        ImGuiWindowFlags_NoSavedSettings |  //
        ImGuiWindowFlags_NoMove |           //
        ImGuiWindowFlags_AlwaysAutoResize;

    ImGui::Begin("Entity Properties", nullptr, window_flags);

    if (state_->selected_name.empty()) {
        ImGui::TextDisabled("No entity selected");
    } else {
        auto ent = state_->name_to_entity[state_->selected_name];

        auto& world = engine_->get_world();
        auto has_model = world.has_component<gfx::model_component>(ent);

        ImGui::Text("Selected: %s %u.%u", state_->selected_name.c_str(), ent.index, ent.generation);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (!world.has_component<gfx::transform_component>(ent)) {
            ImGui::TextColored(ImVec4(1.f, 1.f, 0.f, 1.f), "Entity has no transform component");
        } else {
            render_position();
            render_rotation();
            render_scale();
            render_origin();
        }
    }

    ImGui::Dummy({200.0f, 0.0f});

    state_->ui.right_side_voffset += ImGui::GetWindowHeight() + 10.0f;

    ImGui::End();
}

inline void entity_properties_panel::render_position() {
    auto ent = state_->name_to_entity[state_->selected_name];

    auto& world          = engine_->get_world();
    auto& transform_comp = world.get_component<gfx::transform_component>(ent);
    vec3f position       = transform_comp.get_position();
    if (render_vec3f_field("Position", position)) {
        transform new_transform = transform_comp.get_transform();
        new_transform.set_position(position);

        set_transform_params params = {
            .name      = state_->selected_name,
            .transform = new_transform,
        };
        auto op = std::make_unique<set_transform_operation>(*engine_, *state_, params);
        op_manager_->execute(std::move(op));
    }
}

inline void entity_properties_panel::render_rotation() {
    auto ent = state_->name_to_entity[state_->selected_name];

    auto& world          = engine_->get_world();
    auto& transform_comp = world.get_component<gfx::transform_component>(ent);
    vec3f rotation       = transform_comp.get_rotation();
    vec3f rotation_deg   = {
        math::degrees(rotation.x),
        math::degrees(rotation.y),
        math::degrees(rotation.z),
    };

    if (render_vec3f_field("Rotation", rotation_deg)) {
        vec3f rotation_rad = {
            math::radians(rotation_deg.x),
            math::radians(rotation_deg.y),
            math::radians(rotation_deg.z),
        };
        transform new_transform = transform_comp.get_transform();
        new_transform.set_rotation(rotation_rad);

        set_transform_params params = {
            .name      = state_->selected_name,
            .transform = new_transform,
        };
        auto op = std::make_unique<set_transform_operation>(*engine_, *state_, params);
        op_manager_->execute(std::move(op));
    }
}

inline void entity_properties_panel::render_scale() {
    auto ent = state_->name_to_entity[state_->selected_name];

    auto& world          = engine_->get_world();
    auto& transform_comp = world.get_component<gfx::transform_component>(ent);
    vec3f scale          = transform_comp.get_scale();
    if (render_vec3f_field("Scale", scale)) {
        transform new_transform = transform_comp.get_transform();
        new_transform.set_scale(scale);

        set_transform_params params = {
            .name      = state_->selected_name,
            .transform = new_transform,
        };
        auto op = std::make_unique<set_transform_operation>(*engine_, *state_, params);
        op_manager_->execute(std::move(op));
    }
}

inline void entity_properties_panel::render_origin() {
    auto ent = state_->name_to_entity[state_->selected_name];

    auto& world          = engine_->get_world();
    auto& transform_comp = world.get_component<gfx::transform_component>(ent);
    vec3f origin         = transform_comp.get_origin();
    if (render_vec3f_field("Origin", origin)) {
        transform new_transform = transform_comp.get_transform();
        new_transform.set_origin(origin);

        set_transform_params params = {
            .name      = state_->selected_name,
            .transform = new_transform,
        };
        auto op = std::make_unique<set_transform_operation>(*engine_, *state_, params);
        op_manager_->execute(std::move(op));
    }
}

inline bool entity_properties_panel::render_vec3f_field(
    const char* label, vec3f& vec
) {
    bool vec_changed = false;

    ImGui::PushID(label);

    ImGui::AlignTextToFramePadding();
    std::string text = std::format("{}:", label);
    ImGui::Text("%s", text.c_str());

    ImGui::AlignTextToFramePadding();
    ImGui::Text("X");
    ImGui::SameLine();

    ImGui::PushItemWidth(80.0f);
    std::string drag_x_id = std::format("##{}X", label);
    vec_changed |= ImGui::DragFloat(drag_x_id.c_str(), &vec.x, 0.1f, 0, 0, "%.4f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Y");
    ImGui::SameLine();

    ImGui::PushItemWidth(80.0f);
    std::string drag_y_id = std::format("##{}Y", label);
    vec_changed |= ImGui::DragFloat(drag_y_id.c_str(), &vec.y, 0.1f, 0, 0, "%.4f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Z");
    ImGui::SameLine();

    ImGui::PushItemWidth(80.0f);
    std::string drag_z_id = std::format("##{}Z", label);
    vec_changed |= ImGui::DragFloat(drag_z_id.c_str(), &vec.z, 0.1f, 0, 0, "%.4f");
    ImGui::PopItemWidth();

    ImGui::PopID();

    return vec_changed;
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_ENTITY_PROPERTIES_PANEL_INL_H
