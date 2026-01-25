#pragma once

#ifndef VW_SCULPTOR_PAINT_VOXEL_OPERATION_H
#define VW_SCULPTOR_PAINT_VOXEL_OPERATION_H

#include <string>

#include "app/app_state.h"
#include "base_operation.h"
#include "vw/core/color.h"
#include "vw/core/vec3.h"
#include "vw/gfx/engine/engine.h"

namespace vw::sculptor {

struct paint_voxel_params {
    std::string name;
    vec3i position;
    color color;
};

class paint_voxel_operation final : public base_operation {
public:
    using engine_type = gfx::engine<>;

    paint_voxel_operation(engine_type& eng, app_state& st, const paint_voxel_params& params);

    void execute() override;
    void undo() override;

private:
    engine_type* engine_;
    app_state* state_;
    paint_voxel_params params_;
    color previous_color_;
};

}  // namespace vw::sculptor

#include "paint_voxel_operation.inl.h"

#endif  // VW_SCULPTOR_PAINT_VOXEL_OPERATION_H
