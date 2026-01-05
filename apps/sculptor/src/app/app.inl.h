#pragma once

#ifndef VW_SCULPTOR_APP_INL_H
#define VW_SCULPTOR_APP_INL_H

namespace vw::sculptor {

inline app::app(
    gfx::engine<>& eng
)
    : gfx::app<>(eng) {}

inline void app::render(
    float delta_time
) {
    gfx::app<>::render(delta_time);
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_APP_INL_H
