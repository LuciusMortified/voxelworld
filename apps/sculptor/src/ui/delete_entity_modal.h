#pragma once

#ifndef VW_SCULPTOR_DELETE_ENTITY_MODAL_H
#define VW_SCULPTOR_DELETE_ENTITY_MODAL_H

#include <vw/gfx/engine/engine.h>

#include "app/app_state.h"
#include "operations/operation_manager.h"
#include "operations/delete_entity_operation.h"

namespace vw::sculptor {

class delete_entity_modal final {
public:
    using engine_type = gfx::engine;

    delete_entity_modal(engine_type& eng, app_state& state, operation_manager& op_manager);

    void open(const std::string& delete_name);

    void render(float delta_time);

private:
    engine_type* engine_;
    app_state* state_;
    operation_manager* op_manager_;

    bool need_open_ = false;

    std::string delete_name_;
};

}  // namespace vw::sculptor

#include "delete_entity_modal.inl.h"

#endif  // VW_SCULPTOR_DELETE_ENTITY_MODAL_H
