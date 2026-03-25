#pragma once

#ifndef VW_SCULPTOR_ENTITY_PROPERTIES_PANEL_INL_H
#define VW_SCULPTOR_ENTITY_PROPERTIES_PANEL_INL_H

namespace vw::sculptor {

inline entity_properties_panel::entity_properties_panel(
    engine_type& eng, app_state& st, operation_manager& op_manager
)
    : engine_(&eng)
    , state_(&st)
    , op_manager_(&op_manager)
    , add_model_modal_(eng, st, op_manager) {}

inline void entity_properties_panel::render(float /*delta_time*/) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const auto window_pos         = ImVec2(
        viewport->WorkPos.x + viewport->WorkSize.x - 10,
        viewport->WorkPos.y + state_->ui.right_top_voffset + 10
    );
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(1.0f, 0.0f));

    constexpr ImGuiWindowFlags window_flags =  //
        ImGuiWindowFlags_NoSavedSettings |     //
        ImGuiWindowFlags_NoMove |              //
        ImGuiWindowFlags_AlwaysAutoResize;

    ImGui::Begin("Entity Properties", nullptr, window_flags);

    if (state_->scene.selected_name.empty()) {
        ImGui::TextDisabled("No entity selected");
    } else {
        const auto ent    = state_->scene.name_to_entity[state_->scene.selected_name];
        const auto& world = engine_->get_world();

        ImGui::Text(
            "Selected: %s %u.%u",
            state_->scene.selected_name.c_str(),
            ent.index,
            ent.generation
        );

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (!world.has_component<gfx::transform_component>(ent)) {
            ImGui::TextColored(ImVec4(1.f, 1.f, 0.f, 1.f), "Entity has no transform component");
        } else {
            ImGui::BeginDisabled(state_->anim.animation_mode);
            render_position();
            render_rotation();
            render_scale();
            render_origin();
            ImGui::EndDisabled();
        }

        ImGui::Spacing();
        render_components_section();
    }

    ImGui::Dummy({200.0f, 0.0f});

    state_->ui.right_top_voffset += ImGui::GetWindowHeight() + 10.0f;

    add_model_modal_.render();

    ImGui::End();
}

inline void entity_properties_panel::render_position() const {
    const auto ent             = state_->scene.name_to_entity[state_->scene.selected_name];
    auto& world                = engine_->get_world();
    const auto& transform_comp = world.get_component<gfx::transform_component>(ent);
    vec3f position             = transform_comp.get_position();
    if (imgui_drag_vec3f("Pos", position)) {
        transform new_transform = transform_comp.get_transform();
        new_transform.set_position(position);
        set_transform_params params = {
            .name          = state_->scene.selected_name,
            .new_transform = new_transform,
        };
        op_manager_->execute(std::make_unique<set_transform_operation>(*engine_, *state_, params));
    }
}

inline void entity_properties_panel::render_rotation() const {
    const auto ent             = state_->scene.name_to_entity[state_->scene.selected_name];
    auto& world                = engine_->get_world();
    const auto& transform_comp = world.get_component<gfx::transform_component>(ent);
    const vec3f rotation_euler = transform_comp.get_rotation_euler();
    vec3f rotation_deg         = {
        math::degrees(rotation_euler.x),
        math::degrees(rotation_euler.y),
        math::degrees(rotation_euler.z),
    };
    if (imgui_drag_vec3f("Rot", rotation_deg)) {
        const vec3f rotation_rad = {
            math::radians(rotation_deg.x),
            math::radians(rotation_deg.y),
            math::radians(rotation_deg.z),
        };
        transform new_transform = transform_comp.get_transform();
        new_transform.set_rotation_euler(rotation_rad);
        set_transform_params params = {
            .name          = state_->scene.selected_name,
            .new_transform = new_transform,
        };
        op_manager_->execute(std::make_unique<set_transform_operation>(*engine_, *state_, params));
    }
}

inline void entity_properties_panel::render_scale() const {
    const auto ent       = state_->scene.name_to_entity[state_->scene.selected_name];
    auto& world          = engine_->get_world();
    auto& transform_comp = world.get_component<gfx::transform_component>(ent);
    vec3f scale          = transform_comp.get_scale();
    if (imgui_drag_vec3f("Scale", scale)) {
        transform new_transform = transform_comp.get_transform();
        new_transform.set_scale(scale);
        set_transform_params params = {
            .name          = state_->scene.selected_name,
            .new_transform = new_transform,
        };
        op_manager_->execute(std::make_unique<set_transform_operation>(*engine_, *state_, params));
    }
}

inline void entity_properties_panel::render_origin() const {
    const auto ent             = state_->scene.name_to_entity[state_->scene.selected_name];
    auto& world                = engine_->get_world();
    const auto& transform_comp = world.get_component<gfx::transform_component>(ent);
    vec3f origin               = transform_comp.get_origin();
    if (imgui_drag_vec3f("Origin", origin)) {
        transform new_transform = transform_comp.get_transform();
        new_transform.set_origin(origin);
        set_transform_params params = {
            .name          = state_->scene.selected_name,
            .new_transform = new_transform,
        };
        op_manager_->execute(std::make_unique<set_transform_operation>(*engine_, *state_, params));
    }
}

inline void entity_properties_panel::render_components_section() {
    if (!ImGui::CollapsingHeader("Components")) {
        return;
    }

    const auto& name = state_->scene.selected_name;
    const auto ent   = state_->scene.name_to_entity.at(name);
    auto& world      = engine_->get_world();

    ImGui::BeginDisabled(state_->anim.animation_mode);

    const bool has_model = world.has_component<gfx::model_component>(ent);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Model");
    ImGui::SameLine(100.f);
    if (has_model) {
        if (ImGui::Button("Remove##model")) {
            op_manager_->execute(std::make_unique<remove_model_component_operation>(
                *engine_, *state_, remove_model_component_params{.name = name}
            ));
        }
    } else {
        if (ImGui::Button("Add##model")) {
            add_model_modal_.open(name);
        }
    }

    const bool has_socket = world.has_component<gfx::socket_component>(ent);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Socket");
    ImGui::SameLine(100.f);
    if (has_socket) {
        if (ImGui::Button("Remove##socket")) {
            op_manager_->execute(std::make_unique<remove_socket_component_operation>(
                *engine_, *state_, remove_socket_component_params{.name = name}
            ));
        }
    } else {
        if (ImGui::Button("Add##socket")) {
            op_manager_->execute(std::make_unique<add_socket_component_operation>(
                *engine_, *state_, add_socket_component_params{.name = name}
            ));
        }
    }

    const bool has_anim_target = world.has_component<gfx::animation_target_component>(ent);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Anim. Target");
    ImGui::SameLine(100.f);
    if (has_anim_target) {
        const auto& target_comp = world.get_component<gfx::animation_target_component>(ent);
        ImGui::TextDisabled("(%s)", target_comp.get_name().c_str());
        ImGui::SameLine();
        if (ImGui::Button("Remove##anim_target")) {
            op_manager_->execute(std::make_unique<remove_animation_target_operation>(
                *engine_, *state_, remove_animation_target_params{.entity_name = name}
            ));
        }
    } else {
        if (ImGui::Button("Add##anim_target")) {
            op_manager_->execute(std::make_unique<add_animation_target_operation>(
                *engine_, *state_,
                add_animation_target_params{.entity_name = name, .target_name = name}
            ));
        }
    }

    ImGui::EndDisabled();
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_ENTITY_PROPERTIES_PANEL_INL_H
