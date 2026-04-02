#pragma once

#ifndef VW_SCULPTOR_SAVE_CLIP_AS_MODAL_H
#define VW_SCULPTOR_SAVE_CLIP_AS_MODAL_H

#include "app/app_state.h"
#include "services/clip_service.h"
#include "vw/gfx/engine/engine.h"

namespace vw::sculptor {

class save_clip_as_modal final {
public:
    using engine_type = gfx::engine<>;

    save_clip_as_modal(engine_type& eng, app_state& st, clip_service& clip_svc);

    void open();
    void render(float delta_time);

private:
    void render_overwrite_confirmation_();
    void render_save_form_();
    auto save_clip_() -> bool;

    engine_type* engine_;
    app_state* state_;
    clip_service* clip_service_;

    std::string name_;
    std::string error_;

    bool need_open_                  = false;
    bool need_overwrite_confirmation_ = false;
    bool has_overwrite_confirmation_  = false;
};

}  // namespace vw::sculptor

#include "save_clip_as_modal.inl.h"

#endif  // VW_SCULPTOR_SAVE_CLIP_AS_MODAL_H
