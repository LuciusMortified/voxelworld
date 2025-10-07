#pragma once

#ifndef VW_GFX_ENGINE_H
#define VW_GFX_ENGINE_H

#include <string_view>
#include <memory>
#include <chrono>

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

        engine(const engine&) = delete;
        engine& operator=(const engine&) = delete;

        void run(std::unique_ptr<app> app);
        void shutdown();

        [[nodiscard]]
        window& get_window() const { return *window_; }

        [[nodiscard]]
        vulkan_context& get_vulkan_context() const { return *vulkan_context_; }

        [[nodiscard]]
        renderer& get_renderer() const { return *renderer_; }

        [[nodiscard]]
        camera& get_camera() const { return *camera_; }

        [[nodiscard]]
        world& get_world() const { return *world_; }

        [[nodiscard]]
        debug_window& get_debug_tool() const { return *debug_tool_; }

    private:
        void main_loop();

        void update(float delta_time) const;
        void render() const;

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
} // namespace vw::gfx

#endif // VW_GFX_ENGINE_H
