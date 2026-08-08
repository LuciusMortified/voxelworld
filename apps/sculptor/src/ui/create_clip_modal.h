#pragma once

#ifndef VW_SCULPTOR_CREATE_CLIP_MODAL_H
#define VW_SCULPTOR_CREATE_CLIP_MODAL_H

#include <vw/gfx/engine/engine.h>

#include "app/app_state.h"
#include "operations/operation_manager.h"

namespace vw::sculptor {

class create_clip_modal final {
public:
    using engine_type = gfx::engine;

    create_clip_modal(engine_type& eng, app_state& state, operation_manager& op_manager);

    void open();
    void render(float delta_time);

private:
    bool create_clip();

    engine_type* engine_;
    app_state* state_;
    operation_manager* op_manager_;

    bool need_open_                  = false;
    bool need_overwrite_confirmation_ = false;
    bool has_overwrite_confirmation_  = false;
    std::string name_;
    std::string error_;
};

}  // namespace vw::sculptor

#include "create_clip_modal.inl.h"

#endif  // VW_SCULPTOR_CREATE_CLIP_MODAL_H
