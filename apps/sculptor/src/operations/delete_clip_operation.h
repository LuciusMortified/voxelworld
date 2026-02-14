#pragma once

#ifndef VW_SCULPTOR_DELETE_CLIP_OPERATION_H
#define VW_SCULPTOR_DELETE_CLIP_OPERATION_H

#include <vw/gfx/animation/animation_clip.h>
#include <vw/gfx/engine/engine.h>

#include "app/app_state.h"
#include "base_operation.h"

namespace vw::sculptor {

struct delete_clip_params {
    std::string name;
};

class delete_clip_operation final : public base_operation {
public:
    using engine_type = gfx::engine<>;

    delete_clip_operation(engine_type& engine, app_state& state, const delete_clip_params& params);

    void execute() override;
    void undo() override;

private:
    engine_type* engine_;
    app_state* state_;
    delete_clip_params params_;
    std::shared_ptr<gfx::animation_clip> saved_clip_;
};

}  // namespace vw::sculptor

#include "delete_clip_operation.inl.h"

#endif  // VW_SCULPTOR_DELETE_CLIP_OPERATION_H
