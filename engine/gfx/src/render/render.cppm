export module vw.gfx:render;

// Собирает партиции кадрового пути: контекст устройства, таймер стадий, карта
// теней и компьютный отсев.
export import :render.vulkan_context;
export import :render.gpu_timer;
export import :render.shadow_map;
export import :render.cull_pipeline;
