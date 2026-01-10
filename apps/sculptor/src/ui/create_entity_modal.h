#pragma once

#ifndef VW_SCULPTOR_CREATE_ENTITY_MODAL_H
#define VW_SCULPTOR_CREATE_ENTITY_MODAL_H

#include <vw/gfx/engine/engine.h>
#include <vw/gfx/world/world_components.h>

#include "app/app_state.h"
#include "operations/create_entity_operation.h"

namespace vw::sculptor {

template <typename WC = gfx::base_world_components>
class create_entity_modal final {
public:
    using engine_type = gfx::engine<WC>;
    using operation_type = create_entity_operation<WC>;

    create_entity_modal(engine_type& eng, app_state& state, operation_manager& op_manager);

    void open();

    void render(float delta_time);

private:
    engine_type* engine_;
    app_state* state_;
    operation_manager* op_manager_;

    bool need_open_ = false;

    std::string name_;
    vec3i size_{6, 6, 6};

    std::string error_;

    bool create_entity();
};

}  // namespace vw::sculptor

#include "create_entity_modal.inl.h"

#endif  // VW_SCULPTOR_CREATE_ENTITY_MODAL_H
