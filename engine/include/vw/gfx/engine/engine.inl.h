#pragma once

#ifndef VW_GFX_ENGINE_INL_H
#define VW_GFX_ENGINE_INL_H

#include <vector>

#include "vw/core/timing.h"
#include "vw/gfx/engine/engine.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#include <Psapi.h>
#endif

namespace vw::gfx {

inline engine::engine(
    int width, int height, std::string_view title
) {
    window_         = std::make_unique<window>(width, height, title);
    vulkan_context_ = std::make_unique<vulkan_context>(*window_);
    renderer_       = std::make_unique<renderer_type>(*vulkan_context_, *window_, block_registry_);
    camera_ =
        std::make_unique<camera>(45.0f, static_cast<float>(width) / static_cast<float>(height));
    world_      = std::make_unique<world_type>();
    debug_tool_ = std::make_unique<debug_window_type>(*this);

    // Default empty app to avoid null checks
    app_ = std::make_unique<app_type>(*this);

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
        using keys = keyboard::keys;
        using mods = keyboard::mods;

        if (event.key == keys::F12 && event.with(mods::CTRL)) {
            debug_tool_->toggle_visibility();
        }
        return false;
    });

    last_memory_update_time_ = std::chrono::high_resolution_clock::now();
}

inline engine::~engine() {
    shutdown();
}

template <typename TApp, typename... TArgs>
void engine::run(
    TArgs&&... args
) {
    app_ = std::make_unique<TApp>(*this, std::forward<TArgs>(args)...);
    main_loop();
}

inline void engine::shutdown() {
    running_ = false;
    renderer_->wait_idle();
}

inline auto engine::get_window() const -> window& {
    return *window_;
}

inline auto engine::get_vulkan_context() const -> vulkan_context& {
    return *vulkan_context_;
}

inline auto engine::get_renderer() const -> renderer_type& {
    return *renderer_;
}

inline auto engine::get_camera() const -> camera& {
    return *camera_;
}

inline auto engine::get_world() const -> world_type& {
    return *world_;
}

inline auto engine::get_block_registry() const -> const block_registry& {
    return block_registry_;
}

inline auto engine::get_debug_tool() const -> debug_window_type& {
    return *debug_tool_;
}

inline void engine::main_loop() {
    running_         = true;
    last_frame_time_ = std::chrono::high_resolution_clock::now();

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
}

inline void engine::render(
    float delta_time
) {
    stats_.world_update_ms = measure_ms([&] { world_->update(delta_time); });

    stats_.world_render_ms = measure_ms([&] {
        stats_.begin_frame_ms = measure_ms([&] { renderer_->begin_frame(); });
        stats_.app_render_ms  = measure_ms([&] {
            app_->render(delta_time);
            debug_tool_->render(delta_time);
        });
        stats_.renderer_ms  = measure_ms([&] { renderer_->render(*world_, *camera_); });
        stats_.end_frame_ms = measure_ms([&] { renderer_->end_frame(); });
    });

    world_->clear_changed();
}

inline const engine_stats& engine::get_stats() const {
    return stats_;
}

inline void engine::update_stats() {
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

inline auto engine::calculate_ram_usage() -> uint64 {
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

inline auto engine::calculate_vram_usage() const -> uint64 {
    const auto& vk_context                 = *vulkan_context_;
    const VkPhysicalDevice physical_device = vk_context.get_physical_device();

    VkPhysicalDeviceMemoryBudgetPropertiesEXT budget_props{};
    budget_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;

    VkPhysicalDeviceMemoryProperties2 mem_props{};
    mem_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
    mem_props.pNext = &budget_props;

    auto vkGetPhysicalDeviceMemoryProperties2 =
        reinterpret_cast<PFN_vkGetPhysicalDeviceMemoryProperties2KHR>(
            vkGetInstanceProcAddr(vk_context.get_instance(), "vkGetPhysicalDeviceMemoryProperties2")
        );
    if (vkGetPhysicalDeviceMemoryProperties2 == nullptr) {
        vkGetPhysicalDeviceMemoryProperties2 =
            reinterpret_cast<PFN_vkGetPhysicalDeviceMemoryProperties2KHR>(vkGetInstanceProcAddr(
                vk_context.get_instance(), "vkGetPhysicalDeviceMemoryProperties2KHR"
            ));
    }

    if (vkGetPhysicalDeviceMemoryProperties2 != nullptr) {
        vkGetPhysicalDeviceMemoryProperties2(physical_device, &mem_props);

        uint64 total_usage = 0;
        for (uint32 i = 0; i < mem_props.memoryProperties.memoryHeapCount; ++i) {
            if ((mem_props.memoryProperties.memoryHeaps[i].flags &
                 VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0) {
                total_usage += budget_props.heapUsage[i];
            }
        }
        return total_usage;
    }

    return 0;
}

}  // namespace vw::gfx

#endif  // VW_GFX_ENGINE_INL_H
