#pragma once

#ifndef VW_SCULPTOR_ADD_TRACK_OPERATION_H
#define VW_SCULPTOR_ADD_TRACK_OPERATION_H

#include <optional>

#include <vw/gfx/engine/engine.h>

#include "app/app_state.h"
#include "base_operation.h"

namespace vw::sculptor {

struct add_track_params {
    std::string clip_name;
    std::string track_name;
    std::optional<gfx::animation_property> property;
    std::optional<keyframe_value> keyframe;
};

class add_track_operation final : public base_operation {
public:
    using engine_type = gfx::engine<>;

    add_track_operation(engine_type& engine, app_state& state, const add_track_params& params);

    void execute() override;
    void undo() override;

private:
    engine_type* engine_;
    app_state* state_;
    add_track_params params_;
    bool added_target_component_ = false;
};

}  // namespace vw::sculptor

#include "add_track_operation.inl.h"

#endif  // VW_SCULPTOR_ADD_TRACK_OPERATION_H
