export module vw.gfx:renderer.stats;

import std;

import vw.core;
import :render.gpu_timer;
import :resource;

export namespace vw::gfx {

struct render_timing_stats {
    gpu_timing_stats gpu{};

    // Сбор готовых мешей и заказ новых. Раньше входил в общий итог рендерера и
    // больше никуда — так всплеск в 46 мс и прятался за стадиями, каждая из которых
    // показывала однозначные числа.
    float32 mesh_sync_ms            = 0.0f;
    float32 shadow_map_update_ms    = 0.0f;
    float32 buffer_pool_update_ms   = 0.0f;
    float32 compute_cull_ms         = 0.0f;

    // Построение списка видимых источников на CPU и диспатч, который его заменит.
    // Обе стадии названы до того, как хоть одна из них что-то значит: стадия,
    // появившаяся вместе с изменением, цену которого она должна показать, не имеет
    // с чем сравниваться.
    float32 light_gather_ms         = 0.0f;
    float32 light_cull_ms           = 0.0f;

    float32 shadow_pass_ms          = 0.0f;
    float32 world_pass_ms           = 0.0f;
    float32 world_pass_uniform_ms   = 0.0f;
    float32 world_pass_geometry_ms  = 0.0f;
    float32 world_pass_debug_ms     = 0.0f;
    float32 world_pass_imgui_ms     = 0.0f;
    float32 shadow_cascades_drawn   = 0.0f;
};

struct renderer_stats {
    combined_buffer_pool_stats combined_buffers;
    uint32 draw_call_count = 0;
    render_timing_stats timing;
};
}  // namespace vw::gfx
