#pragma once

#ifndef VW_SCULPTOR_DUMMY_TOOL_H
#define VW_SCULPTOR_DUMMY_TOOL_H

#include "base_tool.h"

namespace vw::sculptor {

class dummy_tool final : public base_tool {
public:
    void render(
        float delta_time
    ) override {}

    void on_key_press(
        const gfx::key_press_event& ev
    ) override {}

    void on_mouse_move(
        const gfx::mouse_move_event& ev
    ) override {}

    void on_mouse_press(
        const gfx::mouse_press_event& ev
    ) override {}

    void on_mouse_release(
        const gfx::mouse_release_event& ev
    ) override {}

    void on_activate() override {}
};

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_DUMMY_TOOL_H
