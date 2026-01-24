#pragma once

#ifndef VW_SCULPTOR_ENTITY_TREE_PANEL_INL_H
#define VW_SCULPTOR_ENTITY_TREE_PANEL_INL_H

namespace vw::sculptor {

inline entity_tree_panel::entity_tree_panel(
    engine_type& eng, app_state& st, operation_manager& op_manager
)
    : engine_(&eng)
    , state_(&st)
    , op_manager_(&op_manager)
    , creation_modal_(eng, st, op_manager)
    , deletion_modal_(eng, st, op_manager) {}

inline void entity_tree_panel::render(
    float delta_time
) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 window_pos       = ImVec2(
        viewport->WorkPos.x + viewport->WorkSize.x - 10,
        viewport->WorkPos.y + state_->ui.right_side_voffset + 10
    );
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(1.0f, 0.0f));

    ImGuiWindowFlags window_flags =          //
        ImGuiWindowFlags_NoCollapse |        //
        ImGuiWindowFlags_NoSavedSettings |   //
        ImGuiWindowFlags_AlwaysAutoResize |  //
        ImGuiWindowFlags_NoMove;

    ImGui::Begin("Entity Tree", nullptr, window_flags);

    if (state_->root_name.empty()) {
        if (ImGui::Button("New root")) {
            creation_modal_.open();
        }
    }

    if (!state_->root_name.empty()) {
        render_entity_node(state_->root_name);
    }

    creation_modal_.render(delta_time);
    deletion_modal_.render(delta_time);

    ImGui::Dummy({200.0f, 0.0f});

    ImGui::End();
}

inline void entity_tree_panel::render_entity_node(
    const std::string& name
) {
    if (name.empty()) {
        return;
    }

    auto ent    = state_->name_to_entity[name];
    auto& world = engine_->get_world();

    bool has_hierarchy = world.has_component<gfx::hierarchy_component>(ent);
    bool has_children  = false;

    if (has_hierarchy) {
        const auto& hierarchy_comp = world.get_component<gfx::hierarchy_component>(ent);
        has_children               = !hierarchy_comp.get_children().empty();
    }

    bool is_selected = state_->selected_name == name;
    bool is_open     = false;

    ImGuiTreeNodeFlags node_flags =       //
        ImGuiTreeNodeFlags_OpenOnArrow |  //
        ImGuiTreeNodeFlags_OpenOnDoubleClick |
        ImGuiTreeNodeFlags_DefaultOpen;

    if (is_selected) {
        node_flags |= ImGuiTreeNodeFlags_Selected;
    }

    auto node_id = std::format("{}##{}.{}", name, ent.index, ent.generation);
    if (has_children) {
        if (ImGui::TreeNodeEx(node_id.c_str(), node_flags)) {
            is_open = true;
        }
    } else {
        if (ImGui::Selectable(node_id.c_str(), is_selected)) {
            state_->selected_name = name;
        }
    }

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        state_->selected_name = name;
    }

    auto context_menu_id = std::format("EntityContextMenu_{}_{}", ent.index, ent.generation);
    if (ImGui::BeginPopupContextItem(context_menu_id.c_str())) {
        state_->selected_name = name;
        if (ImGui::MenuItem("Create child")) {
            creation_modal_.open();
        }
        if (!has_children && ImGui::MenuItem("Delete entity")) {
            deletion_modal_.open(name);
        }
        ImGui::EndPopup();
    }

    if (is_open && has_children) {
        const auto& hierarchy_comp = world.get_component<gfx::hierarchy_component>(ent);
        const auto& children       = hierarchy_comp.get_children();

        for (auto child : children) {
            auto child_name = state_->entity_to_name[child];
            render_entity_node(child_name);
        }

        ImGui::TreePop();
    }
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_ENTITY_TREE_PANEL_INL_H
