#pragma once

#ifndef VW_GFX_ENGINE_H
#define VW_GFX_ENGINE_H

#include <chrono>
#include <memory>
#include <string_view>

#include "vw/gfx/camera/camera.h"
#include "vw/gfx/debug/debug_window.h"
#include "vw/gfx/engine/app.h"
#include "vw/gfx/render/renderer.h"
#include "vw/gfx/render/vulkan_context.h"
#include "vw/platform/event.h"
#include "vw/platform/window.h"
#include "vw/ecs/world.h"

namespace vw::gfx {

using namespace ::vw::ecs;
using namespace ::vw::plat;

struct engine_stats {
    float32 fps             = 0.0f;
    float32 frame_ms        = 0.0f;
    float32 world_update_ms = 0.0f;
    float32 world_render_ms = 0.0f;
    float32 begin_frame_ms  = 0.0f;
    float32 app_render_ms   = 0.0f;
    float32 renderer_ms     = 0.0f;
    float32 end_frame_ms    = 0.0f;
    uint64 ram_usage_bytes  = 0;
    uint64 vram_usage_bytes = 0;
};

class engine final {
public:
    using renderer_type     = renderer;
    using world_type        = world;
    using debug_window_type = debug_window;
    using app_type          = app;

    engine(int width, int height, std::string_view title);
    ~engine();

    engine(const engine&)                    = delete;
    auto operator=(const engine&) -> engine& = delete;

    template <typename TApp, typename... TArgs>
    void run(TArgs&&... args);

    void shutdown();

    [[nodiscard]] auto get_window() const -> window&;
    [[nodiscard]] auto get_vulkan_context() const -> vulkan_context&;
    [[nodiscard]] auto get_renderer() const -> renderer_type&;
    [[nodiscard]] auto get_camera() const -> camera&;
    [[nodiscard]] auto get_world() const -> world_type&;
    [[nodiscard]] auto get_block_registry() const -> const block_registry&;
    [[nodiscard]] auto get_debug_tool() const -> debug_window_type&;
    [[nodiscard]] const engine_stats& get_stats() const;

private:
    void main_loop();

    void render(float delta_time);

    std::unique_ptr<window> window_;
    std::unique_ptr<vulkan_context> vulkan_context_;
    std::unique_ptr<renderer_type> renderer_;
    std::unique_ptr<camera> camera_;
    block_registry block_registry_;
    std::unique_ptr<world_type> world_;
    std::unique_ptr<debug_window_type> debug_tool_;

    std::unique_ptr<app_type> app_;

    bool running_ = false;
    std::chrono::high_resolution_clock::time_point last_frame_time_;

    mutable engine_stats stats_;
    std::chrono::high_resolution_clock::time_point frame_start_time_;
    std::chrono::high_resolution_clock::time_point last_memory_update_time_;

    static constexpr float MEMORY_UPDATE_INTERVAL_SEC = 1.0f;

    void update_stats();
    [[nodiscard]] static uint64 calculate_ram_usage();
    [[nodiscard]] uint64 calculate_vram_usage() const;

    event_sub<window_resize_event> window_resize_sub_;
    event_sub<key_press_event> key_press_sub_;
};

}  // namespace vw::gfx

#include "vw/gfx/debug/debug_window.inl.h"
#include "vw/gfx/engine/engine.inl.h"

#endif  // VW_GFX_ENGINE_H
