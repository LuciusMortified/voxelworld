#pragma once

#ifndef VW_SCULPTOR_ADD_VOXEL_OPERATION_H
#define VW_SCULPTOR_ADD_VOXEL_OPERATION_H

#include <string>

#include "app/app_state.h"
#include "base_operation.h"
#include "vw/core/color.h"
#include "vw/core/vec3.h"
#include "vw/gfx/engine/engine.h"

namespace vw::sculptor {

struct add_voxel_params {
    std::string name;
    vec3i position;
    block_id new_block;
};

class add_voxel_operation final : public base_operation {
public:
    using engine_type = gfx::engine<>;

    add_voxel_operation(engine_type& eng, app_state& st, const add_voxel_params& params);

    void execute() override;
    void undo() override;

private:
    engine_type* engine_;
    app_state* state_;
    add_voxel_params params_;
};

}  // namespace vw::sculptor

#include "add_voxel_operation.inl.h"

#endif  // VW_SCULPTOR_ADD_VOXEL_OPERATION_H
