#pragma once

#ifndef VW_SCULPTOR_NEW_FILE_MODAL_H
#define VW_SCULPTOR_NEW_FILE_MODAL_H

#include "app/app_state.h"
#include "vw/gfx/engine/engine.h"

namespace vw::sculptor {

class new_file_modal final {
public:
    using engine_type = gfx::engine;

    new_file_modal(engine_type& eng, app_state& st);

    void render(float delta_time);

private:
    void render_overwrite_confirmation();
    void render_create_form();
    auto create_file_() -> bool;

    engine_type* engine_;
    app_state* state_;

    std::string filename_;
    std::string error_;

    bool need_overwrite_confirmation_ = false;
    bool has_overwrite_confirmation_  = false;
};

}  // namespace vw::sculptor

#include "new_file_modal.inl.h"

#endif  // VW_SCULPTOR_NEW_FILE_MODAL_H
