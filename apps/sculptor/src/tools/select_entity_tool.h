#pragma once

#ifndef VW_SCULPTOR_SELECT_ENTITY_TOOL_H
#define VW_SCULPTOR_SELECT_ENTITY_TOOL_H

#include <vw/gfx/engine/engine.h>

#include "app/app_state.h"
#include "base_tool.h"

namespace vw::sculptor {

class select_entity_tool final : public base_tool {
public:
    using engine_type = gfx::engine<>;

    select_entity_tool(engine_type& eng, app_state& st);

    void render(float delta_time) override;
    void on_key_press(const gfx::key_press_event& ev) override;
    void on_mouse_move(const gfx::mouse_move_event& ev) override;
    void on_mouse_press(const gfx::mouse_press_event& ev) override;
    void on_mouse_release(const gfx::mouse_release_event& ev) override;
    void on_activate() override;

private:
    engine_type* engine_;
    app_state* state_;

    std::unordered_set<gfx::entity> ray_cast_entities_;
};

}  // namespace vw::sculptor

#include "select_entity_tool.inl.h"

#endif  // VW_SCULPTOR_SELECT_ENTITY_TOOL_H
