#pragma once

#ifndef VW_SCULPTOR_ENTITY_TREE_PANEL_H
#define VW_SCULPTOR_ENTITY_TREE_PANEL_H

#include <algorithm>

#include "app/app_state.h"
#include "create_entity_modal.h"
#include "delete_entity_modal.h"

namespace vw::sculptor {

class entity_tree_panel final {
public:
    using engine_type = gfx::engine;

    entity_tree_panel(engine_type& eng, app_state& st, operation_manager& op_manager);

    void render(float delta_time);

private:
    engine_type* engine_;
    app_state* state_;
    operation_manager* op_manager_;

    create_entity_modal creation_modal_;
    delete_entity_modal deletion_modal_;

    void render_entity_node(
        const std::string& name, const std::unordered_set<ecs::entity>& preview_entities
    );
};

}  // namespace vw::sculptor

#include "entity_tree_panel.inl.h"

#endif  // VW_SCULPTOR_ENTITY_TREE_PANEL_H
