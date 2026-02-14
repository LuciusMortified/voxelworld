#pragma once

#ifndef VW_SCULPTOR_KEYFRAME_PROPERTIES_PANEL_H
#define VW_SCULPTOR_KEYFRAME_PROPERTIES_PANEL_H

#include <vw/gfx/engine/engine.h>

#include "app/app_state.h"
#include "operations/operation_manager.h"

namespace vw::sculptor {

class keyframe_properties_panel final {
public:
    using engine_type = gfx::engine<>;

    keyframe_properties_panel(engine_type& eng, app_state& st, operation_manager& op_manager);

    void render(float delta_time);

private:
    bool render_vec3f_field(const char* label, vec3f& vec);

    engine_type* engine_;
    app_state* state_;
    operation_manager* op_manager_;
};

}  // namespace vw::sculptor

#include "keyframe_properties_panel.inl.h"

#endif  // VW_SCULPTOR_KEYFRAME_PROPERTIES_PANEL_H
