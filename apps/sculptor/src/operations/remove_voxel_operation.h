#pragma once

#ifndef VW_SCULPTOR_REMOVE_VOXEL_OPERATION_H
#define VW_SCULPTOR_REMOVE_VOXEL_OPERATION_H

#include <vw/gfx/engine/engine.h>
#include <vw/gfx/world/world_components.h>

#include "app/app_state.h"
#include "base_operation.h"

namespace vw::sculptor {

struct remove_voxel_params {
    std::string name;
    vec3i position;
};

template <typename WC = gfx::base_world_components>
class remove_voxel_operation final : public base_operation {
public:
    using engine_type = gfx::engine<WC>;

    remove_voxel_operation(engine_type& eng, app_state& st, const remove_voxel_params& params);

    void execute() override;
    void undo() override;

private:
    engine_type* engine_;
    app_state* state_;
    remove_voxel_params params_;
    color previous_color_;
};

}  // namespace vw::sculptor

#include "remove_voxel_operation.inl.h"

#endif  // VW_SCULPTOR_REMOVE_VOXEL_OPERATION_H
