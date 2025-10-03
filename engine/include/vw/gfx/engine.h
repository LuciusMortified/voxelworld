#pragma once
#include <string_view>
#include <memory>
#include <chrono>

#include "vw/gfx/window.h"
#include "vw/gfx/vulkan_context.h"
#include "vw/gfx/renderer.h"
#include "vw/gfx/camera.h"
#include "vw/gfx/app.h"
#include "vw/gfx/events.h"
#include "vw/gfx/world.h"
#include "vw/gfx/debug_tool.h"

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
        debug_tool& get_debug_tool() const { return *debug_tool_; }

    private:
        void main_loop();

        void update(float delta_time) const;
        void render() const;

        std::unique_ptr<window> window_;
        std::unique_ptr<vulkan_context> vulkan_context_;
        std::unique_ptr<renderer> renderer_;
        std::unique_ptr<camera> camera_;
        std::unique_ptr<world> world_;
        std::unique_ptr<debug_tool> debug_tool_;

        std::unique_ptr<app> app_;
        
        bool running_ = false;
        std::chrono::high_resolution_clock::time_point last_frame_time_;
        
        events::sub_id window_resize_sub_ = 0;
        events::sub_id key_press_sub_ = 0;
    };
} // namespace vw::gfx
