#pragma once

#ifndef VW_SCULPTOR_ADD_MODEL_COMPONENT_MODAL_H
#define VW_SCULPTOR_ADD_MODEL_COMPONENT_MODAL_H

#include <vw/gfx/engine/engine.h>

#include "app/app_state.h"
#include "operations/operation_manager.h"

namespace vw::sculptor {

class add_model_component_modal final {
public:
    using engine_type = gfx::engine;

    add_model_component_modal(engine_type& eng, app_state& state, operation_manager& op_manager);

    void open(const std::string& entity_name);
    void render();

private:
    auto confirm_() -> bool;

    engine_type* engine_;
    app_state* state_;
    operation_manager* op_manager_;

    bool need_open_ = false;
    std::string entity_name_;
    vec3i size_{8, 8, 8};
    std::string error_;
};

}  // namespace vw::sculptor

#include "add_model_component_modal.inl.h"

#endif  // VW_SCULPTOR_ADD_MODEL_COMPONENT_MODAL_H
