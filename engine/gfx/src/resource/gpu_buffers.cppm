export module vw.gfx:gpu_buffers;

// Собирает низкоуровневые обёртки над Vulkan: шейдер, буферы, отложенное
// удаление и загрузочный буфер.
export import :resource.shader;
export import :resource.buffer;
export import :resource.deletion_queue;
export import :resource.staging_buffer;
