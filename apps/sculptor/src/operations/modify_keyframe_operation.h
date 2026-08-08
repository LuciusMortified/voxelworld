#pragma once

#ifndef VW_SCULPTOR_MODIFY_KEYFRAME_OPERATION_H
#define VW_SCULPTOR_MODIFY_KEYFRAME_OPERATION_H

#include <vw/gfx/engine/engine.h>

#include "app/app_state.h"
#include "base_operation.h"

namespace vw::sculptor {

struct modify_keyframe_params {
    std::string clip_name;
    std::string track_name;
    asset::animation_property property;
    keyframe_value old_keyframe;
    keyframe_value new_keyframe;
};

class modify_keyframe_operation final : public base_operation {
public:
    using engine_type = gfx::engine;

    modify_keyframe_operation(
        engine_type& engine, app_state& state, const modify_keyframe_params& params
    );

    void execute() override;
    void undo() override;

private:
    void apply(const keyframe_value& replacement) const;

    engine_type* engine_;
    app_state* state_;
    modify_keyframe_params params_;
};

}  // namespace vw::sculptor

#include "modify_keyframe_operation.inl.h"

#endif  // VW_SCULPTOR_MODIFY_KEYFRAME_OPERATION_H
