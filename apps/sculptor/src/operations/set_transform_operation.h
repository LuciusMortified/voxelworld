#pragma once

#ifndef VW_SCULPTOR_SET_TRANSFORM_OPERATION_H
#define VW_SCULPTOR_SET_TRANSFORM_OPERATION_H

#include <vw/gfx/engine/engine.h>

#include "app/app_state.h"
#include "base_operation.h"

namespace vw::sculptor {

struct set_transform_params {
    std::string name;
    transform new_transform;
};

class set_transform_operation final : public base_operation {
public:
    using engine_type = gfx::engine;

    set_transform_operation(engine_type& engine, app_state& st, const set_transform_params& params);

    void execute() override;
    void undo() override;

private:
    engine_type* engine_;
    app_state* state_;
    set_transform_params params_;
    transform previous_transform_;
};

}  // namespace vw::sculptor

#include "set_transform_operation.inl.h"

#endif  // VW_SCULPTOR_SET_TRANSFORM_OPERATION_H
