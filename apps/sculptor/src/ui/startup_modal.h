#pragma once

#ifndef VW_SCULPTOR_STARTUP_MODAL_H
#define VW_SCULPTOR_STARTUP_MODAL_H

#include <string>

#include "app/app_state.h"
#include "ui_utils.h"
#include "vw/gfx/engine/engine.h"

namespace vw::sculptor {

class startup_modal final {
public:
    using engine_type = gfx::engine;

    startup_modal(engine_type& eng, app_state& state);

    void render(float delta_time);

private:
    engine_type* engine_;
    app_state* state_;

    bool need_open_ = false;
};

}  // namespace vw::sculptor

#include "startup_modal.inl.h"

#endif  // VW_SCULPTOR_STARTUP_MODAL_H
