#pragma once

#ifndef VW_SCULPTOR_PAINT_TOOL_H
#define VW_SCULPTOR_PAINT_TOOL_H

#include "app/app_state.h"
#include "base_tool.h"
#include "operations/operation_manager.h"
#include "vw/gfx/engine/engine.h"

namespace vw::sculptor {

class paint_tool final : public base_tool {
public:
    using engine_type = gfx::engine<>;

    paint_tool(engine_type& eng, app_state& st, operation_manager& op_manager);

    void render(float delta_time) override;
    void on_key_press(const gfx::key_press_event& ev) override;
    void on_mouse_move(const gfx::mouse_move_event& ev) override;
    void on_mouse_press(const gfx::mouse_press_event& ev) override;
    void on_mouse_release(const gfx::mouse_release_event& ev) override;
    void on_activate() override;

private:
    void update_hovered_voxel_();

    engine_type* engine_;
    app_state* state_;
    operation_manager* op_manager_;

    std::vector<gfx::entity> ray_cast_entities_;
    vec3i hovered_voxel_ = vec3i{-1, -1, -1};
};

}  // namespace vw::sculptor

#include "paint_tool.inl.h"

#endif  // VW_SCULPTOR_PAINT_TOOL_H
