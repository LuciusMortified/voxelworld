#pragma once

#ifndef VW_GFX_DEBUG_WINDOW_H
#define VW_GFX_DEBUG_WINDOW_H

#include <array>
#include <cfloat>
#include <string>
#include <unordered_map>

#include "vw/core.h"

namespace vw::gfx {

template <typename WC>
class engine;

template <typename WC>
class debug_window final {
public:
    using engine_type = engine<WC>;

    explicit debug_window(engine_type& engine);
    ~debug_window() = default;

    debug_window(const debug_window&)            = delete;
    auto operator=(const debug_window&) -> debug_window& = delete;

    debug_window(debug_window&&)            = default;
    debug_window& operator=(debug_window&&) = default;

    void render(float delta_time);

    void toggle_visibility();
    void set_visible(bool visible);

    [[nodiscard]] bool is_visible() const;

private:
    void render_fps_window();
    void render_render_mode_controls() const;
    void render_combined_buffers_detail();
    void render_systems_detail();
    void render_render_detail();

    engine_type* engine_;

    bool visible_                      = false;
    bool show_combined_buffers_detail_ = false;
    bool show_systems_detail_          = false;
    bool show_render_detail_           = false;

    std::unordered_map<std::string, float32> metric_max_;

    static constexpr size_t fps_bucket_count_      = 200;
    static constexpr float32 fps_bucket_duration_ms_ = 50.0f;

    std::array<float32, fps_bucket_count_> fps_bucket_max_{};
    std::array<float32, fps_bucket_count_> fps_bucket_min_{};
    size_t  fps_bucket_index_         = 0;
    size_t  fps_bucket_filled_count_  = 0;
    float32 fps_bucket_accum_ms_      = 0.0f;
    float32 fps_bucket_current_max_   = 0.0f;
    float32 fps_bucket_current_min_   = FLT_MAX;
};

}  // namespace vw::gfx

#include "vw/gfx/debug/debug_window.inl.h"

#endif  // VW_GFX_DEBUG_WINDOW_H
