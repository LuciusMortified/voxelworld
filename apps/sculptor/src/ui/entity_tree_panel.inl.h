#pragma once

#ifndef VW_SCULPTOR_ENTITY_TREE_PANEL_INL_H
#define VW_SCULPTOR_ENTITY_TREE_PANEL_INL_H

namespace vw::sculptor {

template <typename WC>
entity_tree_panel<WC>::entity_tree_panel(
    engine_type& eng, state& st, creation_tool_type& creation_tool
)
    : engine_(&eng), state_(&st), creation_tool_(&creation_tool) {}

template <typename WC>
void entity_tree_panel<WC>::render(
    float delta_time
) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 window_pos       = ImVec2(
        viewport->WorkPos.x + viewport->WorkSize.x - 10,
        viewport->WorkPos.y + viewport->WorkSize.y - 10
    );
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(1.0f, 1.0f));

    ImGuiWindowFlags window_flags =         //
        ImGuiWindowFlags_NoCollapse |       //
        ImGuiWindowFlags_NoSavedSettings |  //
        ImGuiWindowFlags_NoTitleBar |       //
        ImGuiWindowFlags_NoMove |           //
        ImGuiWindowFlags_AlwaysAutoResize;

    ImGui::Begin("Entity Tree", nullptr, window_flags);

    render_title();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (state_->root_entity.is_valid()) {
        render_entity_node(state_->root_entity);
    }

    render_create_root_dialog();
    render_context_menu();

    ImGui::End();
}

template <typename WC>
void entity_tree_panel<WC>::render_title() {
    ImGui::Text("Entity Tree: ");
    ImGui::SameLine();

    if (!state_->root_entity.is_valid()) {
        if (ImGui::Button("+ New root")) {
            ImGui::OpenPopup("Create Root Entity");
        }
    }
}

template <typename WC>
void entity_tree_panel<WC>::render_entity_node(
    gfx::entity ent
) {
    if (!ent.is_valid()) {
        return;
    }

    auto& ent_state = state_->entity_map[ent];
    auto& world     = engine_->get_world();

    bool has_hierarchy = world.template has_component<gfx::hierarchy_component>(ent);
    bool has_children  = false;

    if (has_hierarchy) {
        const auto& hierarchy_comp = world.template get_component<gfx::hierarchy_component>(ent);
        has_children               = !hierarchy_comp.get_children().empty();
    }

    if (ent_state.depth > 0) {
        ImGui::Indent(15.0f * ent_state.depth);
    }

    bool is_open     = open_nodes_.contains(ent);
    bool is_selected = state_->selected_entity == ent;

    ImGuiTreeNodeFlags node_flags =       //
        ImGuiTreeNodeFlags_OpenOnArrow |  //
        ImGuiTreeNodeFlags_OpenOnDoubleClick;

    if (is_selected) {
        node_flags |= ImGuiTreeNodeFlags_Selected;
    }
    if (!has_children) {
        node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    if (has_children) {
        if (ImGui::TreeNodeEx(ent_state.name.c_str(), node_flags)) {
            open_nodes_.insert(ent);
        }
    } else {
        if (ImGui::Selectable(ent_state.name.c_str(), is_selected)) {
            state_->selected_entity = ent;
        }
    }

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        state_->selected_entity = ent;
    }

    if (is_open && has_children) {
        const auto& hierarchy_comp = world.template get_component<gfx::hierarchy_component>(ent);
        const auto& children       = hierarchy_comp.get_children();

        for (auto child : children) {
            render_entity_node(child);
        }

        ImGui::TreePop();
    }

    if (ent_state.depth > 0) {
        ImGui::Unindent(15.0f * ent_state.depth);
    }
}

template <typename WC>
void entity_tree_panel<WC>::render_create_root_dialog() {
    bool is_open =
        ImGui::BeginPopupModal("Create Root Entity", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    if (is_open) {
        static char name_buffer[128] = "root";
        ImGui::InputText("Name", name_buffer, sizeof(name_buffer));

        static int size_x = 6;
        static int size_y = 6;
        static int size_z = 6;
        ImGui::InputInt("Size X", &size_x);
        ImGui::InputInt("Size Y", &size_y);
        ImGui::InputInt("Size Z", &size_z);

        if (ImGui::Button("Create")) {
            creation_tool_->execute({
                .name       = name_buffer,
                .parent     = gfx::invalid_entity,
                .model_size = vec3i{size_x, size_y, size_z},
            });
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

template <typename WC>
void entity_tree_panel<WC>::render_context_menu() {
    if (ImGui::BeginPopupContextWindow("EntityContextMenu")) {
        if (ImGui::MenuItem("Delete entity")) {
            // TODO: delete entity
        }
        if (ImGui::MenuItem("Create child")) {
            // TODO: create child entity
        }
        ImGui::EndPopup();
    }
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_ENTITY_TREE_PANEL_INL_H
