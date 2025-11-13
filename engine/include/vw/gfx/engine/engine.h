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
#include "vw/gfx/window/event.h"
#include "vw/gfx/window/window.h"
#include "vw/gfx/world/world.h"

namespace vw::gfx {
class engine final {
public:
    engine(int width, int height, std::string_view title);
    ~engine();

    engine(const engine&)                    = delete;
    auto operator=(const engine&) -> engine& = delete;

    void run(std::unique_ptr<app> app);
    void shutdown();

    [[nodiscard]]
    auto get_window() const -> window& {
        return *window_;
    }

    [[nodiscard]]
    auto get_vulkan_context() const -> vulkan_context& {
        return *vulkan_context_;
    }

    [[nodiscard]]
    auto get_renderer() const -> renderer& {
        return *renderer_;
    }

    [[nodiscard]]
    auto get_camera() const -> camera& {
        return *camera_;
    }

    [[nodiscard]]
    auto get_world() const -> world& {
        return *world_;
    }

    [[nodiscard]]
    auto get_debug_tool() const -> debug_window& {
        return *debug_tool_;
    }

private:
    void main_loop();

    void render(float delta_time) const;

    std::unique_ptr<window> window_;
    std::unique_ptr<vulkan_context> vulkan_context_;
    std::unique_ptr<renderer> renderer_;
    std::unique_ptr<camera> camera_;
    std::unique_ptr<world> world_;
    std::unique_ptr<debug_window> debug_tool_;

    std::unique_ptr<app> app_;

    bool running_ = false;
    std::chrono::high_resolution_clock::time_point last_frame_time_;

    event_sub<window_resize_event> window_resize_sub_;
    event_sub<key_press_event> key_press_sub_;
};
}  // namespace vw::gfx

#endif  // VW_GFX_ENGINE_H
