#pragma once

#ifndef VW_SCULPTOR_CREATE_KEYFRAME_MODAL_H
#define VW_SCULPTOR_CREATE_KEYFRAME_MODAL_H

#include <vw/gfx/engine/engine.h>

#include "app/app_state.h"
#include "operations/operation_manager.h"

namespace vw::sculptor {

class create_keyframe_modal final {
public:
    using engine_type = gfx::engine;

    create_keyframe_modal(engine_type& eng, app_state& st, operation_manager& op_manager);

    void open(const std::string& track_name);
    void render(float delta_time);

private:
    bool create_keyframe();

    engine_type* engine_;
    app_state* state_;
    operation_manager* op_manager_;

    bool need_open_ = false;
    std::string track_name_;
    int property_index_     = 0;
    float32 time_           = 0.f;
    vec3f value_vec3f_      = {};
    vec3f value_euler_deg_  = {};
    int interp_index_       = 0;
    float32 tangent_in_     = 0.f;
    float32 tangent_out_    = 1.f;
    std::string error_;
};

}  // namespace vw::sculptor

#include "create_keyframe_modal.inl.h"

#endif  // VW_SCULPTOR_CREATE_KEYFRAME_MODAL_H
