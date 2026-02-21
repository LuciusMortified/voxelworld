#pragma once

#ifndef VW_SCULPTOR_CREATE_ENTITY_MODAL_H
#define VW_SCULPTOR_CREATE_ENTITY_MODAL_H

#include <vw/gfx/engine/engine.h>

#include "app/app_state.h"

namespace vw::sculptor {

class create_entity_modal final {
public:
    using engine_type = gfx::engine<>;

    create_entity_modal(engine_type& eng, app_state& state, operation_manager& op_manager);

    void open();

    void render(float delta_time);

private:
    auto create_entity() -> bool;

    engine_type* engine_;
    app_state* state_;
    operation_manager* op_manager_;

    bool need_open_ = false;

    std::string name_;
    bool with_model_  = false;
    bool with_socket_ = false;
    vec3i size_{12, 12, 12};

    std::string error_;
};

}  // namespace vw::sculptor

#include "create_entity_modal.inl.h"

#endif  // VW_SCULPTOR_CREATE_ENTITY_MODAL_H
