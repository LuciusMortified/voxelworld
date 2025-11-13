#pragma once

#ifndef VW_GFX_DEBUG_WINDOW_H
#define VW_GFX_DEBUG_WINDOW_H

#include <chrono>
#include <vector>

#include "vw/core.h"
#include "vw/gfx/render/renderer.h"

namespace vw::gfx {
class debug_window final {
public:
    explicit debug_window(renderer& renderer);
    ~debug_window() = default;

    debug_window(const debug_window&)            = delete;
    debug_window& operator=(const debug_window&) = delete;

    debug_window(debug_window&&)            = default;
    debug_window& operator=(debug_window&&) = default;

    void render(float delta_time);

    void toggle_visibility();
    void set_visible(bool visible) {
        visible_ = visible;
    }

    [[nodiscard]]
    bool is_visible() const {
        return visible_;
    }

private:
    void update_fps(float delta_time);
    void update_fps_history(float fps);
    void render_fps_window();
    void render_fps_graph() const;
    void render_render_mode_controls() const;

    renderer* renderer_;

    bool visible_      = false;
    float current_fps_ = 0.0f;
    float average_fps_ = 0.0f;
    float min_fps_     = 0.0f;
    float max_fps_     = 0.0f;

    std::vector<float> fps_history_;
    static constexpr uint32 MAX_HISTORY_SIZE = 120;

    std::chrono::high_resolution_clock::time_point last_fps_update_;
    static constexpr float FPS_UPDATE_INTERVAL = 0.1f;

    float fps_accumulator_ = 0.0f;
    uint32 frame_count_    = 0;
};

}  // namespace vw::gfx

#endif  // VW_GFX_DEBUG_WINDOW_H
