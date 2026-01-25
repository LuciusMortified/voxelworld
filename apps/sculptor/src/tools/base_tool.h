#pragma once

#ifndef VW_SCULPTOR_BASE_TOOL_H
#define VW_SCULPTOR_BASE_TOOL_H

#include <vw/gfx/window/event.h>

namespace vw::sculptor {

class base_tool {
public:
    virtual ~base_tool() = default;

    virtual void render(float delta_time) = 0;

    virtual void on_key_press(const gfx::key_press_event& ev)         = 0;
    virtual void on_mouse_move(const gfx::mouse_move_event& ev)       = 0;
    virtual void on_mouse_press(const gfx::mouse_press_event& ev)     = 0;
    virtual void on_mouse_release(const gfx::mouse_release_event& ev) = 0;

    virtual void on_activate() = 0;
};

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_BASE_TOOL_H
