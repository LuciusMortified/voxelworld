#pragma once

#ifndef VW_SCULPTOR_ENTITY_PROPERTIES_PANEL_H
#define VW_SCULPTOR_ENTITY_PROPERTIES_PANEL_H

#include "add_model_component_modal.h"
#include "app/app_state.h"
#include "operations/add_animation_target_operation.h"
#include "operations/add_socket_component_operation.h"
#include "operations/remove_animation_target_operation.h"
#include "operations/remove_model_component_operation.h"
#include "operations/remove_socket_component_operation.h"
#include "operations/set_transform_operation.h"
#include "ui_utils.h"

namespace vw::sculptor {

class entity_properties_panel final {
public:
    using engine_type = gfx::engine<>;

    entity_properties_panel(engine_type& eng, app_state& st, operation_manager& op_manager);

    void render(float delta_time);

private:
    engine_type* engine_;
    app_state* state_;
    operation_manager* op_manager_;

    add_model_component_modal add_model_modal_;

    mutable std::string cached_rotation_entity_;
    mutable quat cached_rotation_quat_;
    mutable vec3f cached_rotation_deg_;

    void render_components_section();

    void render_position() const;
    void render_rotation() const;
    void render_scale() const;
    void render_origin() const;
};

}  // namespace vw::sculptor

#include "entity_properties_panel.inl.h"

#endif  // VW_SCULPTOR_ENTITY_PROPERTIES_PANEL_H
