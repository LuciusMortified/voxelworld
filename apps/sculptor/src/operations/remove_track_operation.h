#pragma once

#ifndef VW_SCULPTOR_REMOVE_TRACK_OPERATION_H
#define VW_SCULPTOR_REMOVE_TRACK_OPERATION_H

#include <vw/gfx/animation/animation_track.h>
#include <vw/gfx/engine/engine.h>

#include "app/app_state.h"
#include "base_operation.h"

namespace vw::sculptor {

struct remove_track_params {
    std::string clip_name;
    std::string track_name;
};

class remove_track_operation final : public base_operation {
public:
    using engine_type = gfx::engine<>;

    remove_track_operation(engine_type& eng, app_state& state, remove_track_params params);

    void execute() override;
    void undo() override;

private:
    engine_type* engine_;
    app_state* state_;
    remove_track_params params_;
    std::optional<gfx::animation_track> saved_track_;
};

}  // namespace vw::sculptor

#include "remove_track_operation.inl.h"

#endif  // VW_SCULPTOR_REMOVE_TRACK_OPERATION_H
