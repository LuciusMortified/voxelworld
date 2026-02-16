#pragma once

#ifndef VW_SCULPTOR_ENTITY_PROPERTIES_PANEL_H
#define VW_SCULPTOR_ENTITY_PROPERTIES_PANEL_H

#include "app/app_state.h"
#include "operations/set_transform_operation.h"
#include "ui_utils.h"

namespace vw::sculptor {

class entity_properties_panel final {
public:
    using engine_type    = gfx::engine<>;

    entity_properties_panel(engine_type& eng, app_state& st, operation_manager& op_manager);

    void render(float delta_time);

private:
    engine_type* engine_;
    app_state* state_;
    operation_manager* op_manager_;

    void render_model_info();

    void render_position();
    void render_rotation();
    void render_scale();
    void render_origin();

    bool render_vec3f_field(const char* label, vec3f& vec);
};

}  // namespace vw::sculptor

#include "entity_properties_panel.inl.h"

#endif  // VW_SCULPTOR_ENTITY_PROPERTIES_PANEL_H
