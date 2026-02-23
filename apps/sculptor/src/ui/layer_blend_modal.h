#pragma once

#ifndef VW_SCULPTOR_LAYER_BLEND_MODAL_H
#define VW_SCULPTOR_LAYER_BLEND_MODAL_H

#include "app/app_state.h"

namespace vw::sculptor {

class layer_blend_modal final {
public:
    explicit layer_blend_modal(app_state& st);

    void open();
    void render(float delta_time);

private:
    app_state* state_;

    bool need_open_ = false;

    float fade_in_duration_  = 0.f;
    int fade_in_interp_      = 0;
    float fade_out_duration_ = 0.f;
    int fade_out_interp_     = 0;
};

}  // namespace vw::sculptor

#include "layer_blend_modal.inl.h"

#endif  // VW_SCULPTOR_LAYER_BLEND_MODAL_H
