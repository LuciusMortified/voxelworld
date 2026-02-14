#pragma once

#ifndef VW_SCULPTOR_CLIP_MANAGER_PANEL_H
#define VW_SCULPTOR_CLIP_MANAGER_PANEL_H

#include <vw/gfx/engine/engine.h>

#include "app/app_state.h"
#include "create_clip_modal.h"
#include "operations/operation_manager.h"

namespace vw::sculptor {

class clip_manager_panel final {
public:
    using engine_type = gfx::engine<>;

    clip_manager_panel(engine_type& eng, app_state& st, operation_manager& op_manager);

    void render(float delta_time);

private:
    engine_type* engine_;
    app_state* state_;
    operation_manager* op_manager_;

    create_clip_modal create_modal_;
};

}  // namespace vw::sculptor

#include "clip_manager_panel.inl.h"

#endif  // VW_SCULPTOR_CLIP_MANAGER_PANEL_H
