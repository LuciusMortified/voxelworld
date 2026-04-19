#pragma once

#ifndef VW_SCULPTOR_REMOVE_MODEL_COMPONENT_OPERATION_H
#define VW_SCULPTOR_REMOVE_MODEL_COMPONENT_OPERATION_H

#include <vw/gfx/engine/engine.h>
#include <vw/gfx/world/world_components.h>

#include "app/app_state.h"
#include "base_operation.h"

namespace vw::sculptor {

struct remove_model_component_params {
    std::string name;
};

class remove_model_component_operation final : public base_operation {
public:
    using engine_type = gfx::engine<>;

    remove_model_component_operation(
        engine_type& engine, app_state& state, const remove_model_component_params& params
    );

    void execute() override;
    void undo() override;

private:
    engine_type* engine_;
    app_state* state_;
    remove_model_component_params params_;

    std::shared_ptr<asset::model> saved_model_;
};

}  // namespace vw::sculptor

#include "remove_model_component_operation.inl.h"

#endif  // VW_SCULPTOR_REMOVE_MODEL_COMPONENT_OPERATION_H
