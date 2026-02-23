#pragma once

#ifndef VW_SCULPTOR_FILE_SERVICE_H
#define VW_SCULPTOR_FILE_SERVICE_H

#include "app/app_state.h"
#include "vw/gfx/engine/engine.h"

namespace vw::sculptor {

class file_service final {
public:
    using engine_type = gfx::engine<>;

    file_service(engine_type& eng, app_state& state);

    auto save() -> bool;
    auto save_as(const std::filesystem::path& filepath) -> bool;

private:
    engine_type* engine_;
    app_state* state_;
};

}  // namespace vw::sculptor

#include "file_service.inl.h"

#endif  // VW_SCULPTOR_FILE_SERVICE_H
