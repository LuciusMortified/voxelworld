#pragma once

#ifndef VW_SCULPTOR_ENTITY_TREE_PANEL_H
#define VW_SCULPTOR_ENTITY_TREE_PANEL_H

#include "app/state.h"

namespace vw::sculptor {

template <typename WC = gfx::base_world_components>
class entity_tree_panel {
public:
    using engine_type = gfx::engine<WC>;
    using creation_tool_type = entity_creation_tool<WC>;

    entity_tree_panel(engine_type& eng, state& st, creation_tool_type& creation_tool);

    void render(float delta_time);

private:
    engine_type* engine_;
    state* state_;
    creation_tool_type* creation_tool_;

    bool create_root_dialog_open_ = false;

    std::unordered_set<gfx::entity> open_nodes_;

    void render_title();
    void render_entity_node(gfx::entity);
    void render_create_root_dialog();
    void render_context_menu();
};

}  // namespace vw::sculptor

#include "entity_tree_panel.inl.h"

#endif  // VW_SCULPTOR_ENTITY_TREE_PANEL_H
