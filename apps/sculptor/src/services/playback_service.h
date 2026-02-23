#pragma once

#ifndef VW_SCULPTOR_PLAYBACK_SERVICE_H
#define VW_SCULPTOR_PLAYBACK_SERVICE_H

#include "app/app_state.h"
#include "vw/gfx/engine/engine.h"

namespace vw::sculptor {

class playback_service final {
public:
    using engine_type = gfx::engine<>;

    playback_service(engine_type& eng, app_state& state);

    void toggle_playback();
    void stop_playback();

private:
    engine_type* engine_;
    app_state* state_;
};

}  // namespace vw::sculptor

#include "playback_service.inl.h"

#endif  // VW_SCULPTOR_PLAYBACK_SERVICE_H
