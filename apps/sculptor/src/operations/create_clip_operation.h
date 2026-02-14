#pragma once

#ifndef VW_SCULPTOR_CREATE_CLIP_OPERATION_H
#define VW_SCULPTOR_CREATE_CLIP_OPERATION_H

#include <vw/gfx/engine/engine.h>

#include "app/app_state.h"
#include "base_operation.h"

namespace vw::sculptor {

struct create_clip_params {
    std::string name;
};

class create_clip_operation final : public base_operation {
public:
    using engine_type = gfx::engine<>;

    create_clip_operation(engine_type& engine, app_state& state, const create_clip_params& params);

    void execute() override;
    void undo() override;

private:
    engine_type* engine_;
    app_state* state_;
    create_clip_params params_;
};

}  // namespace vw::sculptor

#include "create_clip_operation.inl.h"

#endif  // VW_SCULPTOR_CREATE_CLIP_OPERATION_H
