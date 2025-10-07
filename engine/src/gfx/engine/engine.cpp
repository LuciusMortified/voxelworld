#include "vw/gfx/engine/engine.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <iostream>

#include "vw/gfx/debug/debug_window.h"
#include "vw/gfx/engine/app.h"
#include "vw/gfx/render/renderer.h"
#include "vw/gfx/render/vulkan_context.h"
#include "vw/gfx/window/window.h"
#include "vw/gfx/world/world.h"
#include "vw/gfx/camera/camera.h"

namespace vw::gfx {

engine::engine(int width, int height, std::string_view title) {
    window_ = std::make_unique<window>(width, height, title);
    vulkan_context_ = std::make_unique<vulkan_context>(*window_);
    renderer_ = std::make_unique<renderer>(*vulkan_context_, *window_);
    camera_ = std::make_unique<camera>(
        45.0f,
        static_cast<float>(width) / static_cast<float>(height)
    );
    world_ = std::make_unique<world>(*vulkan_context_);
    debug_tool_ = std::make_unique<debug_window>(*renderer_);

    // Default empty app to avoid null checks
    app_ = std::make_unique<app>();

    window_resize_sub_ = window_->sub<window_resize_event>(
        [this](const window_resize_event& event) -> bool {
            if (event.width > 0 && event.height > 0) {
                const float aspect = static_cast<float>(event.width) /
                    static_cast<float>(event.height);
                camera_->set_aspect_ratio(aspect);
                renderer_->handle_resize();
            }
            return false;
        }
    );

    key_press_sub_ = window_->sub<key_press_event>(
        [this](const key_press_event& event) -> bool {
            if (event.key == keyboard::key::F12 && event.with(keyboard::mod::CTRL)) {
                debug_tool_->toggle_visibility();
            }
            return false;
        }
    );
}

engine::~engine() {
    shutdown();
}

void engine::run(std::unique_ptr<app> app) {
    if (!app) {
        throw std::runtime_error("invalid app");
    }

    app_ = std::move(app);
    app_->run(*this);

    main_loop();
}

void engine::shutdown() {
    running_ = false;

    app_->cleanup();

    renderer_->wait_idle();
}

void engine::main_loop() {
    running_ = true;
    last_frame_time_ = std::chrono::high_resolution_clock::now();

    app_->setup();

    while (running_ && !window_->should_close()) {
        auto current_time = std::chrono::high_resolution_clock::now();
        auto delta_time =
            std::chrono::duration<float>(current_time - last_frame_time_)
                .count();
        last_frame_time_ = current_time;

        if (delta_time > 0.1f) {
            delta_time = 0.1f;
        }

        window_->poll_events();

        update(delta_time);

        render();
    }

    app_->cleanup();
}

void engine::update(float delta_time) const {
    world_->update_meshes();
    debug_tool_->update(delta_time);
    app_->update(delta_time);
}

void engine::render() const {
    try {
        renderer_->begin_frame();

        debug_tool_->render();

        app_->render();

        renderer_->render(*world_, *camera_);

        renderer_->end_frame();
    } catch (const std::exception& e) {
        std::cerr << "Render error: " << e.what() << std::endl;
    }
}

}  // namespace vw::gfx