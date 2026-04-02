#pragma once

#ifndef VW_SCULPTOR_CLIP_SERVICE_H
#define VW_SCULPTOR_CLIP_SERVICE_H

#include "app/app_state.h"
#include "operations/operation_manager.h"
#include "vw/gfx/engine/engine.h"

namespace vw::sculptor {

class clip_service final {
public:
    using engine_type = gfx::engine<>;

    clip_service(engine_type& eng, app_state& state, operation_manager& op_manager);

    auto save_clip(const std::string& clip_name) const -> bool;
    auto save_clip_as(const std::string& clip_name, const std::string& new_name) const -> bool;
    void save_all_clips() const;
    auto load_clip(const std::string& filename) const -> bool;
    void close_clip(const std::string& clip_name) const;

    void enter_animation_mode();
    void exit_animation_mode();
    void force_exit_animation_mode();

    void save_transforms();
    void restore_transforms();
    void reset_all();

    void stop_layer_for_clip(const std::string& clip_name);
    void stop_all_layers();

private:
    engine_type* engine_;
    app_state* state_;
    operation_manager* op_manager_;
};

}  // namespace vw::sculptor

#include "clip_service.inl.h"

#endif  // VW_SCULPTOR_CLIP_SERVICE_H
