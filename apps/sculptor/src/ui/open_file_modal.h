#pragma once

#ifndef VW_SCULPTOR_OPEN_FILE_MODAL_H
#define VW_SCULPTOR_OPEN_FILE_MODAL_H

#include "app/app_state.h"
#include "vw/gfx/engine/engine.h"

namespace vw::sculptor {

class open_file_modal final {
public:
    using engine_type = gfx::engine;

    open_file_modal(engine_type& eng, app_state& st);

    void render(float delta_time);

private:
    void load_existing_filenames_();
    auto open_file_() -> bool;

    engine_type* engine_;
    app_state* state_;

    std::string filename_;
    std::string error_;
    std::vector<std::string> existing_filenames_;
};

}  // namespace vw::sculptor

#include "open_file_modal.inl.h"

#endif  // VW_SCULPTOR_OPEN_FILE_MODAL_H
