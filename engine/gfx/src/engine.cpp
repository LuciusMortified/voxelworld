module;

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Psapi.h>
#endif

module vw.gfx;

import std;
import vulkan;
import vw.core;
import vw.ecs;
import vw.world;
import vw.platform;
import :vk;

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif


#endif

namespace vw::gfx {

engine::engine(
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

engine::~engine() {
    shutdown();
}

void engine::shutdown() {
    running_ = false;
    renderer_->wait_idle();
}

auto engine::get_window() const -> window& {
    return *window_;
}

auto engine::get_vulkan_context() const -> vulkan_context& {
    return *vulkan_context_;
}

auto engine::get_renderer() const -> renderer_type& {
    return *renderer_;
}

auto engine::get_camera() const -> camera& {
    return *camera_;
}

auto engine::get_world() const -> world_type& {
    return *world_;
}

auto engine::get_block_registry() const -> const block_registry& {
    return block_registry_;
}

auto engine::get_debug_tool() const -> debug_window_type& {
    return *debug_tool_;
}

void engine::main_loop() {
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

void engine::render(
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

const engine_stats& engine::get_stats() const {
    return stats_;
}

void engine::update_stats() {
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

auto engine::calculate_ram_usage() -> uint64 {
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

auto engine::calculate_vram_usage() const -> uint64 {
    const auto chain = vulkan_context_->get_physical_device().getMemoryProperties2<
        vk::PhysicalDeviceMemoryProperties2,
        vk::PhysicalDeviceMemoryBudgetPropertiesEXT>();

    const auto& mem_props = chain.get<vk::PhysicalDeviceMemoryProperties2>().memoryProperties;
    const auto& budget    = chain.get<vk::PhysicalDeviceMemoryBudgetPropertiesEXT>();

    uint64 total_usage = 0;
    for (uint32 i = 0; i < mem_props.memoryHeapCount; ++i) {
        if (mem_props.memoryHeaps[i].flags & vk::MemoryHeapFlagBits::eDeviceLocal) {
            total_usage += budget.heapUsage[i];
        }
    }
    return total_usage;
}

}  // namespace vw::gfx
