#pragma once

#ifndef VW_SCULPTOR_CREATE_ENTITY_OPERATION_H
#define VW_SCULPTOR_CREATE_ENTITY_OPERATION_H

#include <vw/gfx/engine/engine.h>
#include <vw/gfx/world/world_components.h>

#include "app/app_state.h"
#include "base_operation.h"

namespace vw::sculptor {

struct create_entity_params {
    std::string name;
    std::string parent_name;

    bool with_model = false;
    vec3i size      = vec3i{6, 6, 6};
};

template <typename WC = gfx::base_world_components>
class create_entity_operation final : public base_operation {
public:
    using engine_type = gfx::engine<WC>;

    create_entity_operation(
        engine_type& engine, app_state& state, const create_entity_params& params = {}
    );

    void execute() override;
    void undo() override;

private:
    engine_type* engine_;
    app_state* state_;

    create_entity_params params_;
};

}  // namespace vw::sculptor

#include "create_entity_operation.inl.h"

#endif  // VW_SCULPTOR_CREATE_ENTITY_OPERATION_H
