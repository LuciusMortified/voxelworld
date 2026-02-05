#pragma once

#ifndef VW_SCULPTOR_SAVE_AS_MODAL_H
#define VW_SCULPTOR_SAVE_AS_MODAL_H
#include "app/app_state.h"
#include "vw/gfx/engine/engine.h"

namespace vw::sculptor {

class save_as_modal final {
public:
    using engine_type = gfx::engine<>;

    save_as_modal(engine_type& eng, app_state& st);

    void render(float delta_time);

private:
    auto save_file_() -> bool;

    engine_type* engine_;
    app_state* state_;

    std::string filename_;
    std::string error_;

    bool need_overwrite_confirmation_ = false;
    bool has_overwrite_confirmation_  = false;
};

}  // namespace vw::sculptor

#include "save_as_modal.inl.h"

#endif  // VW_SCULPTOR_SAVE_AS_MODAL_H
