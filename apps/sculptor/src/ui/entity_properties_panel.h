#pragma once

#ifndef VW_SCULPTOR_ENTITY_PROPERTIES_PANEL_H
#define VW_SCULPTOR_ENTITY_PROPERTIES_PANEL_H

#include "app/app_state.h"
#include "operations/set_transform_operation.h"
#include "ui_utils.h"

namespace vw::sculptor {

template <typename WC = gfx::base_world_components>
class entity_properties_panel {
public:
    using engine_type    = gfx::engine<WC>;
    using operation_type = set_transform_operation<WC>;

    entity_properties_panel(engine_type& eng, app_state& st, operation_manager& op_manager);

    void render(float delta_time);

private:
    engine_type* engine_;
    app_state* state_;
    operation_manager* op_manager_;

    void render_position();
    void render_rotation();
    void render_scale();
    void render_origin();
    void render_actions();

    bool render_vec3f_field(const char* label, vec3f& vec);
};

}  // namespace vw::sculptor

#include "entity_properties_panel.inl.h"

#endif  // VW_SCULPTOR_ENTITY_PROPERTIES_PANEL_H
