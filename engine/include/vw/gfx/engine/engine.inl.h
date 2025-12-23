#pragma once

#ifndef VW_GFX_ENGINE_INL_H
#define VW_GFX_ENGINE_INL_H

#include <vector>

#include "vw/gfx/engine/engine.h"

#ifdef _WIN32
extern "C" {

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <Psapi.h>
}
#endif

namespace vw::gfx {

template <typename WC>
engine<WC>::engine(
    int width, int height, std::string_view title
) {
    window_         = std::make_unique<window>(width, height, title);
    vulkan_context_ = std::make_unique<vulkan_context>(*window_);
    renderer_       = std::make_unique<renderer_type>(*vulkan_context_, *window_);
    camera_ =
        std::make_unique<camera>(45.0f, static_cast<float>(width) / static_cast<float>(height));
    world_      = std::make_unique<world_type>(*vulkan_context_);
    debug_tool_ = std::make_unique<debug_window_type>(*this);

    // Default empty app to avoid null checks
    app_ = std::make_unique<app_type>();

    window_resize_sub_ =
        window_->sub<window_resize_event>([this](const window_resize_event& event) -> bool {
            if (event.width > 0 && event.height > 0) {
                const float aspect =
                    static_cast<float>(event.width) / static_cast<float>(event.height);
                camera_->set_aspect_ratio(aspect);
                renderer_->handle_resize();
            }
            return false;
        });

    key_press_sub_ = window_->sub<key_press_event>([this](const key_press_event& event) -> bool {
        if (event.key == keyboard::key::F12 && event.with(keyboard::mod::CTRL)) {
            debug_tool_->toggle_visibility();
        }
        return false;
    });

    last_memory_update_time_ = std::chrono::high_resolution_clock::now();
}

template <typename WC>
engine<WC>::~engine() {
    shutdown();
}

template <typename WC>
void engine<WC>::run(
    std::unique_ptr<app_type> app
) {
    app_ = std::move(app);
    app_->run(*this);

    main_loop();
}

template <typename WC>
void engine<WC>::shutdown() {
    running_ = false;

    app_->cleanup();

    renderer_->wait_idle();
}

template <typename WC>
auto engine<WC>::get_window() const -> window& {
    return *window_;
}

template <typename WC>
auto engine<WC>::get_vulkan_context() const -> vulkan_context& {
    return *vulkan_context_;
}

template <typename WC>
auto engine<WC>::get_renderer() const -> renderer_type& {
    return *renderer_;
}

template <typename WC>
auto engine<WC>::get_camera() const -> camera& {
    return *camera_;
}

template <typename WC>
auto engine<WC>::get_world() const -> world_type& {
    return *world_;
}

template <typename WC>
auto engine<WC>::get_debug_tool() const -> debug_window_type& {
    return *debug_tool_;
}

template <typename WC>
void engine<WC>::main_loop() {
    running_         = true;
    last_frame_time_ = std::chrono::high_resolution_clock::now();

    app_->setup();

    while (running_ && !window_->should_close()) {
        frame_start_time_ = std::chrono::high_resolution_clock::now();

        auto current_time = std::chrono::high_resolution_clock::now();
        auto delta_time   = std::chrono::duration<float>(current_time - last_frame_time_).count();
        delta_time        = std::min(delta_time, 0.1f);

        window_->poll_events();

        render(delta_time);

        update_stats();
        last_frame_time_ = current_time;
    }

    app_->cleanup();
}

template <typename WC>
void engine<WC>::render(
    float delta_time
) {
    world_update_start_time_ = std::chrono::high_resolution_clock::now();
    world_->update();
    const auto world_update_end = std::chrono::high_resolution_clock::now();
    stats_.world_update_ms =
        std::chrono::duration<float32>(world_update_end - world_update_start_time_).count() *
        1000.0f;

    renderer_->begin_frame();
    app_->render(delta_time);
    debug_tool_->render(delta_time);
    renderer_->render(*world_, *camera_);
    renderer_->end_frame();
    const auto render_end = std::chrono::high_resolution_clock::now();
    stats_.world_render_ms =
        std::chrono::duration<float32>(render_end - world_update_end).count() * 1000.0f;
}

template <typename WC>
const engine_stats& engine<WC>::get_stats() const {
    return stats_;
}

template <typename WC>
void engine<WC>::update_stats() {
    const auto current_time = std::chrono::high_resolution_clock::now();
    stats_.frame_ms =
        std::chrono::duration<float32>(current_time - frame_start_time_).count() * 1000.0f;

    if (stats_.frame_ms > 0.0f) {
        stats_.fps = 1000.0f / stats_.frame_ms;
    }

    // Обновляем память только раз в секунду
    const auto time_since_last_memory_update =
        std::chrono::duration<float32>(current_time - last_memory_update_time_).count();
    if (time_since_last_memory_update >= MEMORY_UPDATE_INTERVAL_SEC) {
        stats_.ram_usage_bytes   = calculate_ram_usage();
        stats_.vram_usage_bytes  = calculate_vram_usage();
        last_memory_update_time_ = current_time;
    }
}

template <typename WC>
uint64 engine<WC>::calculate_ram_usage() const {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc{};
    if (GetProcessMemoryInfo(
            GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc)
        )) {
        return pmc.WorkingSetSize;
    }
#endif
    return 0;
}

template <typename WC>
uint64 engine<WC>::calculate_vram_usage() const {
    auto& vk_context                 = *vulkan_context_;
    VkPhysicalDevice physical_device = vk_context.get_physical_device();

    VkPhysicalDeviceMemoryBudgetPropertiesEXT budget_props{};
    budget_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;

    VkPhysicalDeviceMemoryProperties2 mem_props{};
    mem_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
    mem_props.pNext = &budget_props;

    auto vkGetPhysicalDeviceMemoryProperties2 =
        reinterpret_cast<PFN_vkGetPhysicalDeviceMemoryProperties2KHR>(vkGetInstanceProcAddr(
            vk_context.get_instance(), "vkGetPhysicalDeviceMemoryProperties2KHR"
        ));

    if (vkGetPhysicalDeviceMemoryProperties2 != nullptr) {
        vkGetPhysicalDeviceMemoryProperties2(physical_device, &mem_props);

        uint64 total_usage = 0;
        for (uint32 i = 0; i < mem_props.memoryProperties.memoryHeapCount; ++i) {
            if ((mem_props.memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT
                ) != 0) {
                total_usage += budget_props.heapUsage[i];
            }
        }
        return total_usage;
    }

    return 0;
}

}  // namespace vw::gfx

#endif  // VW_GFX_ENGINE_INL_H
