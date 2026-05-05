#pragma once

#ifndef VW_SCULPTOR_DELETE_ENTITY_OPERATION_H
#define VW_SCULPTOR_DELETE_ENTITY_OPERATION_H

#include <vw/gfx/engine/engine.h>
#include <vw/ecs/base_world_def.h>

#include "app/app_state.h"
#include "base_operation.h"

namespace vw::sculptor {

struct delete_entity_params {
    std::string name;
};

class delete_entity_operation final : public base_operation {
public:
    using engine_type = gfx::engine<>;

    delete_entity_operation(
        engine_type& engine, app_state& state, const delete_entity_params& params
    );

    void execute() override;
    void undo() override;

private:
    engine_type* engine_;
    app_state* state_;
    delete_entity_params params_;

    std::string parent_name_;
    transform transform_;
    bool with_model_ = false;
    std::shared_ptr<asset::model> saved_model_;
};

}  // namespace vw::sculptor

#include "delete_entity_operation.inl.h"

#endif  // VW_SCULPTOR_DELETE_ENTITY_OPERATION_H
