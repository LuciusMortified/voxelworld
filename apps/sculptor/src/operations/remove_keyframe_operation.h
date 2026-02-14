#pragma once

#ifndef VW_SCULPTOR_REMOVE_KEYFRAME_OPERATION_H
#define VW_SCULPTOR_REMOVE_KEYFRAME_OPERATION_H

#include <vw/gfx/engine/engine.h>

#include "app/app_state.h"
#include "base_operation.h"

namespace vw::sculptor {

struct remove_keyframe_params {
    std::string clip_name;
    std::string track_name;
    gfx::animation_property property;
    keyframe_value keyframe;
};

class remove_keyframe_operation final : public base_operation {
public:
    using engine_type = gfx::engine<>;

    remove_keyframe_operation(
        engine_type& engine, app_state& state, const remove_keyframe_params& params
    );

    void execute() override;
    void undo() override;

private:
    engine_type* engine_;
    app_state* state_;
    remove_keyframe_params params_;
};

}  // namespace vw::sculptor

#include "remove_keyframe_operation.inl.h"

#endif  // VW_SCULPTOR_REMOVE_KEYFRAME_OPERATION_H
