#pragma once

#ifndef VW_SCULPTOR_RESIZE_MODEL_OPERATION_H
#define VW_SCULPTOR_RESIZE_MODEL_OPERATION_H

#include "app/app_state.h"
#include "base_operation.h"
#include "vw/gfx/engine/engine.h"

namespace vw::sculptor {

struct expand_model_params {
    std::string name;
    vec3i dir{0,0,1};
};

class expand_model_operation final : public base_operation {
public:
    using engine_type = gfx::engine<>;

    expand_model_operation(engine_type& eng, app_state& st, const expand_model_params& params);

    void execute() override;
    void undo() override;

private:
    engine_type* engine_;
    app_state* state_;
    expand_model_params params_;
    vec3i previous_size_;
};

}

#include "expand_model_operation.inl.h"

#endif  // VW_SCULPTOR_RESIZE_MODEL_OPERATION_H
