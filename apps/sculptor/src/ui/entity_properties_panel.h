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

    void render(float delta_time) const;

private:
    engine_type* engine_;
    app_state* state_;
    operation_manager* op_manager_;

    void render_model_info();

    void render_position() const;
    void render_rotation() const;
    void render_scale() const;
    void render_origin() const;

    static bool render_vec3f_field(std::string_view label, vec3f& vec);
};

}  // namespace vw::sculptor

#include "entity_properties_panel.inl.h"

#endif  // VW_SCULPTOR_ENTITY_PROPERTIES_PANEL_H
