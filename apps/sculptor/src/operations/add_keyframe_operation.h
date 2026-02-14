#pragma once

#ifndef VW_SCULPTOR_ADD_KEYFRAME_OPERATION_H
#define VW_SCULPTOR_ADD_KEYFRAME_OPERATION_H

#include <vw/gfx/engine/engine.h>

#include "app/app_state.h"
#include "base_operation.h"

namespace vw::sculptor {

struct add_keyframe_params {
    std::string clip_name;
    std::string track_name;
    gfx::animation_property property;
    keyframe_value keyframe;
};

class add_keyframe_operation final : public base_operation {
public:
    using engine_type = gfx::engine<>;

    add_keyframe_operation(
        engine_type& engine, app_state& state, const add_keyframe_params& params
    );

    void execute() override;
    void undo() override;

private:
    engine_type* engine_;
    app_state* state_;
    add_keyframe_params params_;
    bool created_channel_ = false;
};

}  // namespace vw::sculptor

#include "add_keyframe_operation.inl.h"

#endif  // VW_SCULPTOR_ADD_KEYFRAME_OPERATION_H
