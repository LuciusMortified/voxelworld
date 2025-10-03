#pragma once

#include <vector>
#include <chrono>

#include "vw/types.h"
#include "vw/gfx/renderer.h"

namespace vw::gfx {
class debug_tool final {
public:
    explicit debug_tool(renderer& renderer);
    ~debug_tool();

    debug_tool(const debug_tool&) = delete;
    debug_tool& operator=(const debug_tool&) = delete;

    void update(float delta_time);
    void render();
    
    void toggle_visibility();
    void set_visible(bool visible) { visible_ = visible; }

    [[nodiscard]]
    bool is_visible() const { return visible_; }

private:
    void update_fps_history(float fps);
    void render_fps_window();
    void render_fps_graph() const;
    void render_render_mode_controls() const;

    renderer* renderer_;

    bool visible_ = false;
    float current_fps_ = 0.0f;
    float average_fps_ = 0.0f;
    float min_fps_ = 0.0f;
    float max_fps_ = 0.0f;
    
    std::vector<float> fps_history_;
    static constexpr uint32 MAX_HISTORY_SIZE = 120;
    
    std::chrono::high_resolution_clock::time_point last_fps_update_;
    static constexpr float FPS_UPDATE_INTERVAL = 0.1f;
    
    float fps_accumulator_ = 0.0f;
    uint32 frame_count_ = 0;
};

} // namespace voxel
