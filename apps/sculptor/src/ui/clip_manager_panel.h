#pragma once

#ifndef VW_SCULPTOR_CLIP_MANAGER_PANEL_H
#define VW_SCULPTOR_CLIP_MANAGER_PANEL_H

#include <vw/gfx/engine/engine.h>

#include "app/app_state.h"
#include "create_clip_modal.h"
#include "operations/operation_manager.h"

namespace vw::sculptor {

class clip_manager_panel final {
public:
    using engine_type = gfx::engine<>;

    clip_manager_panel(engine_type& eng, app_state& st, operation_manager& op_manager);

    void render(float delta_time);

    void save_all_clips();

private:
    void render_close_confirm_popup_();
    void render_load_popup_();
    void close_clip_();
    void load_voxa_filenames_();
    auto save_clip_() -> bool;
    auto load_clip_(const std::string& filename) -> bool;

    engine_type* engine_;
    app_state* state_;
    operation_manager* op_manager_;

    create_clip_modal create_modal_;

    bool need_load_popup_          = false;
    bool need_close_confirm_popup_ = false;
    std::vector<std::string> voxa_filenames_;
    std::string selected_load_filename_;
};

}  // namespace vw::sculptor

#include "clip_manager_panel.inl.h"

#endif  // VW_SCULPTOR_CLIP_MANAGER_PANEL_H
