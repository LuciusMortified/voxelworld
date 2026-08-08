#pragma once

#ifndef VW_SCULPTOR_KEYFRAME_SERVICE_H
#define VW_SCULPTOR_KEYFRAME_SERVICE_H

#include "app/app_state.h"
#include "operations/operation_manager.h"
#include "vw/gfx/engine/engine.h"

namespace vw::sculptor {

class keyframe_service final {
public:
    using engine_type = gfx::engine;

    keyframe_service(engine_type& eng, app_state& state, operation_manager& op_manager);

    void add_keyframe();
    void delete_keyframe();

private:
    engine_type* engine_;
    app_state* state_;
    operation_manager* op_manager_;
};

}  // namespace vw::sculptor

#include "keyframe_service.inl.h"

#endif  // VW_SCULPTOR_KEYFRAME_SERVICE_H
