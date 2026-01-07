#pragma once

#ifndef VW_SCULPTOR_ENTITY_PROPERTIES_PANEL_INL_H
#define VW_SCULPTOR_ENTITY_PROPERTIES_PANEL_INL_H

namespace vw::sculptor {

template <typename WC>
entity_properties_panel<WC>::entity_properties_panel(
    engine_type& eng, state& st
)
    : engine_(&eng), state_(&st) {}

template <typename WC>
void entity_properties_panel<WC>::render(
    float delta_time
) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 window_pos =
        ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - 10, viewport->WorkPos.y + 10);
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(1.0f, 0.0f));

    ImGuiWindowFlags window_flags =         //
        ImGuiWindowFlags_NoCollapse |       //
        ImGuiWindowFlags_NoSavedSettings |  //
        ImGuiWindowFlags_NoTitleBar |       //
        ImGuiWindowFlags_NoMove |           //
        ImGuiWindowFlags_AlwaysAutoResize;

    ImGui::Begin("Entity Properties", nullptr, window_flags);

    if (!state_->selected_entity.is_valid()) {
        ImGui::TextDisabled("No entity selected");
        ImGui::End();
        return;
    }

    render_entity_id();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    render_position();
    render_rotation();
    render_scale();
    render_origin();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    render_actions();

    ImGui::End();
}

template <typename WC>
void entity_properties_panel<WC>::render_entity_id() {
    ImGui::Text(
        "Entity Properties: %u.%u",
        state_->selected_entity.index,
        state_->selected_entity.generation
    );
}

template <typename WC>
void entity_properties_panel<WC>::render_position() {
    auto ent               = state_->selected_entity;
    auto& world            = engine_->get_world();
    auto& transform_system = world.get_transform_system();
    auto& transform_comp   = world.template get_component<gfx::transform_component>(ent);
    vec3f position         = transform_comp.get_position();
    if (render_vec3f_field("Position", position)) {
        transform_system.modify(ent).set_position(position);
    }
}

template <typename WC>
void entity_properties_panel<WC>::render_rotation() {
    auto ent               = state_->selected_entity;
    auto& world            = engine_->get_world();
    auto& transform_system = world.get_transform_system();
    auto& transform_comp   = world.template get_component<gfx::transform_component>(ent);
    vec3f rotation         = transform_comp.get_rotation();
    vec3f rotation_deg     = {
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
        transform_system.modify(ent).set_rotation(rotation_rad);
    }
}

template <typename WC>
void entity_properties_panel<WC>::render_scale() {
    auto ent               = state_->selected_entity;
    auto& world            = engine_->get_world();
    auto& transform_system = world.get_transform_system();
    auto& transform_comp   = world.template get_component<gfx::transform_component>(ent);
    vec3f scale            = transform_comp.get_scale();
    if (render_vec3f_field("Scale", scale)) {
        transform_system.modify(ent).set_scale(scale);
    }
}

template <typename WC>
void entity_properties_panel<WC>::render_origin() {
    auto ent               = state_->selected_entity;
    auto& world            = engine_->get_world();
    auto& transform_system = world.get_transform_system();
    auto& transform_comp   = world.template get_component<gfx::transform_component>(ent);
    vec3f origin           = transform_comp.get_origin();
    if (render_vec3f_field("Origin", origin)) {
        transform_system.modify(ent).set_origin(origin);
    }
}

template <typename WC>
void entity_properties_panel<WC>::render_actions() {
    ImGui::Dummy(ImVec2{ImGui::GetContentRegionAvail().x - 100.f, 0});
    ImGui::SameLine(0, 0);
    ImGui::Button("Reset all", ImVec2{100.f, 20.f});
}

template <typename WC>
bool entity_properties_panel<WC>::render_vec3f_field(
    const char* label, vec3f& vec
) {
    bool vec_changed = false;

    ImGui::PushID(label);

    ImGui::AlignTextToFramePadding();
    std::string text = std::format("{}:", label);
    ImGui::Text(text.c_str());

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
