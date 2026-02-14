#pragma once

#ifndef VW_SCULPTOR_DELETE_TRACK_MODAL_H
#define VW_SCULPTOR_DELETE_TRACK_MODAL_H

#include <vw/gfx/engine/engine.h>

#include "app/app_state.h"
#include "operations/operation_manager.h"

namespace vw::sculptor {

class delete_track_modal final {
public:
    using engine_type = gfx::engine<>;

    delete_track_modal(engine_type& eng, app_state& st, operation_manager& op_manager);

    void open(const std::string& track_name);
    void render(float delta_time);

private:
    engine_type* engine_;
    app_state* state_;
    operation_manager* op_manager_;

    bool need_open_ = false;
    std::string track_name_;
};

}  // namespace vw::sculptor

#include "delete_track_modal.inl.h"

#endif  // VW_SCULPTOR_DELETE_TRACK_MODAL_H
