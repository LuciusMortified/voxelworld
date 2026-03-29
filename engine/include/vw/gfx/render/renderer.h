#pragma once

#ifndef VW_GFX_RENDERER_H
#define VW_GFX_RENDERER_H

#include <vulkan/vulkan.h>

#include <chrono>
#include <memory>
#include <vector>

#include "vw/core.h"
#include "vw/gfx/camera/camera.h"
#include "vw/gfx/debug/debug_primitive.h"
#include "vw/gfx/render/cull_pipeline.h"
#include "vw/gfx/render/shadow_map.h"
#include "vw/gfx/resource/combined_buffer_pool.h"
#include "vw/gfx/resource/light_buffer.h"
#include "vw/gfx/resource/palette_buffer.h"
#include "vw/gfx/resource/shader.h"
#include "vw/gfx/window/window.h"
#include "vw/gfx/world/world.h"

namespace vw::gfx {

enum class render_mode : uint8 { lit, wireframe };

// Настройки directional light (для CPU)
struct directional_light_settings {
    vec3f direction{0.0f, -1.0f, 0.0f};
    vec3f color{1.f, 1.f, 1.f};
    float32 intensity{1.0f};
};

// Настройки тумана (для CPU)
struct fog_settings {
    vec3f color{0.1f, 0.1f, 0.1f};
    float32 near_distance{256.0f};
    float32 far_distance{512.0f};
    bool enabled{true};
};

// Для directional light (в UBO)
struct directional_light_data {
    alignas(16) mat4f light_space_matrices[shadow_map::cascade_count];
    alignas(16) vec4f cascade_splits;
    alignas(16) vec3f direction;
    alignas(16) vec3f color;
    alignas(4) float32 intensity;
};

struct fog_data {
    alignas(16) vec3f color;
    alignas(4) float32 near_distance;
    alignas(4) float32 far_distance;
    alignas(4) uint32 enabled;
};

struct uniform_buffer_object {
    // Камера
    alignas(16) float32 view[16]{};
    alignas(16) float32 projection[16]{};
    alignas(16) vec3f view_pos;

    // Directional light
    alignas(16) directional_light_data directional_light;

    // Point lights count (для расширяемости)
    alignas(4) uint32 point_lights_count{0};

    // Fog
    alignas(16) fog_data fog;
};

struct shadow_push_constant_data {
    alignas(4) uint32 cascade_index = 0;
};

// Uniform buffer для shadow pass (light_space_matrix и cascade_index)
struct shadow_uniform_buffer_object {
    alignas(16) mat4f light_space_matrices[shadow_map::cascade_count];
};

struct push_constant_data {
    alignas(16) float32 model[16]{};
};

struct render_timing_stats {
    float32 shadow_map_update_ms    = 0.0f;
    float32 buffer_pool_update_ms   = 0.0f;
    float32 compute_cull_ms         = 0.0f;
    float32 shadow_pass_ms          = 0.0f;
    float32 world_pass_ms           = 0.0f;
    float32 world_pass_uniform_ms   = 0.0f;
    float32 world_pass_geometry_ms  = 0.0f;
    float32 world_pass_debug_ms     = 0.0f;
    float32 world_pass_imgui_ms     = 0.0f;
};

struct renderer_stats {
    combined_buffer_pool_stats combined_buffers;
    uint32 draw_call_count = 0;
    render_timing_stats timing;
};

template <typename WC = base_world_components>
class renderer final {
public:
    using world_type                = world<WC>;
    using combined_buffer_pool_type = combined_buffer_pool<WC>;
    using light_buffer_type         = light_buffer<WC>;

    renderer(vulkan_context& context, window& window, const block_registry& registry);
    ~renderer();

    renderer(const renderer&)            = delete;
    renderer& operator=(const renderer&) = delete;

    renderer(renderer&&)            = delete;
    renderer& operator=(renderer&&) = delete;

    void begin_frame();
    void render(world_type& world, camera& camera);
    void end_frame();

    void set_clear_color(float r, float g, float b, float a = 1.0f);
    void set_clear_color(vec4f color);

    void wait_idle() const;

    void handle_resize();

    void set_render_mode(render_mode mode);

    [[nodiscard]] auto get_render_mode() const -> render_mode;

    [[nodiscard]] auto get_stats() const -> const renderer_stats&;

    [[nodiscard]] auto get_descriptor_pool() const -> VkDescriptorPool {
        return descriptor_pool_;
    }
    [[nodiscard]] auto get_storage_descriptor_set_layout() const -> VkDescriptorSetLayout {
        return storage_descriptor_set_layout_;
    }

    void draw_line(const vec3f& a, const vec3f& b, color col = colors::red);

    void draw_box(const mat4f& matrix, const vec3f& size, color col = colors::red);
    void draw_box(const transform& transform, const vec3f& size, color col = colors::red);
    void draw_box(const vec3f& position, const vec3f& size, color col = colors::red);

    void draw_grid(
        const mat4f& matrix, float cell_size, int cols, int rows, color clr = colors::red
    );
    void draw_grid(
        const transform& transform, float cell_size, int cols, int rows, color clr = colors::red
    );
    void draw_grid(
        const vec3f& position, float cell_size, int cols, int rows, color clr = colors::red
    );

    [[nodiscard]] auto get_directional_light_settings() -> directional_light_settings&;
    [[nodiscard]] auto get_fog_settings() -> fog_settings&;

    void draw_colliders(world_type& w, color col = colors::green);

    // Получить ImTextureID для shadow map (для отображения в ImGui::Image)
    // В Vulkan это VkDescriptorSet, приведенный к void*
    [[nodiscard]] void* get_shadow_map_texture_id(uint32 cascade_index = 0) const;

private:
    void create_swapchain();
    void create_image_views();
    void create_color_resources();
    void create_depth_resources();
    void create_render_pass();
    void create_descriptor_set_layouts();
    void create_graphics_pipeline();
    void create_wireframe_pipeline();
    void create_shadow_pipeline();
    void create_debug_pipeline();
    void create_framebuffers();
    void create_command_buffers();
    void create_sync_objects();
    void create_uniform_buffers();
    void create_shadow_uniform_buffers();
    void create_descriptor_pool();
    void create_descriptor_sets();
    void create_shadow_descriptor_sets();
    void create_shadow_map_descriptor_sets();

    void init_imgui();
    void setup_imgui_style();
    void create_imgui_descriptor_pool();
    void cleanup_imgui();

    void cleanup_descriptor_pool();
    void cleanup_render_pass();
    void cleanup_descriptor_set_layouts();
    void cleanup_pipelines();
    void cleanup_shadow_pipeline();
    void cleanup_debug_pipeline();
    void cleanup_swapchain();
    void cleanup_color_resources();
    void cleanup_depth_resources();
    void recreate_swapchain();

    [[nodiscard]]
    auto get_max_usable_sample_count() -> VkSampleCountFlagBits;

    void create_point_lights_descriptor_set_layout();
    void cleanup_point_lights_resources();

    void create_palette_descriptor_set_layout();
    void cleanup_palette_resources();

    void update_shadow_uniform_buffer() const;
    void render_shadow_pass(world_type& world, const camera& camera);

    void update_uniform_buffer(const camera& camera) const;
    void render_world_pass(world_type& world, const camera& camera);
    void render_world(world_type& world, const camera& camera);

    void render_debug_primitives();
    void update_debug_vertex_buffer();

    void render_imgui() const;

    [[nodiscard]]
    VkFormat find_depth_format();

    [[nodiscard]]
    VkFormat find_supported_format(
        const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features
    );

    void create_image(
        VkImage& image,
        VkDeviceMemory& image_memory,
        VkExtent2D extent,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT
    );

    [[nodiscard]]
    VkImageView create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags);

    [[nodiscard]]
    uint32 find_memory_type(uint32 typeFilter, VkMemoryPropertyFlags properties);

    [[nodiscard]]
    static VkSurfaceFormatKHR choose_swap_surface_format(
        const std::vector<VkSurfaceFormatKHR>& available_formats
    );

    [[nodiscard]]
    static VkPresentModeKHR choose_swap_present_mode(
        const std::vector<VkPresentModeKHR>& available_present_modes
    );

    [[nodiscard]]
    VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities);

    vulkan_context* context_;
    window* window_;

    // Swapchain
    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    std::vector<VkImage> swapchain_images_;
    VkFormat swapchain_image_format_ = VK_FORMAT_UNDEFINED;
    VkExtent2D swapchain_extent_{};
    std::vector<VkImageView> swapchain_image_views_;

    // MSAA
    VkSampleCountFlagBits msaa_samples_ = VK_SAMPLE_COUNT_1_BIT;
    VkImage color_image_                = VK_NULL_HANDLE;
    VkDeviceMemory color_image_memory_  = VK_NULL_HANDLE;
    VkImageView color_image_view_       = VK_NULL_HANDLE;

    // Depth
    VkImage depth_image_               = VK_NULL_HANDLE;
    VkDeviceMemory depth_image_memory_ = VK_NULL_HANDLE;
    VkImageView depth_image_view_      = VK_NULL_HANDLE;

    // Render pass и pipeline
    VkRenderPass render_pass_                                 = VK_NULL_HANDLE;
    VkDescriptorSetLayout uniform_descriptor_set_layout_      = VK_NULL_HANDLE;
    VkDescriptorSetLayout storage_descriptor_set_layout_      = VK_NULL_HANDLE;
    VkDescriptorSetLayout shadow_descriptor_set_layout_       = VK_NULL_HANDLE;
    VkDescriptorSetLayout point_lights_descriptor_set_layout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout palette_descriptor_set_layout_      = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout_                         = VK_NULL_HANDLE;
    VkPipeline graphics_pipeline_                             = VK_NULL_HANDLE;
    VkPipeline wireframe_pipeline_                            = VK_NULL_HANDLE;
    VkPipeline shadow_pipeline_                               = VK_NULL_HANDLE;
    VkPipelineLayout shadow_pipeline_layout_                  = VK_NULL_HANDLE;

    // Framebuffers и команды
    std::vector<VkFramebuffer> framebuffers_;
    std::vector<VkCommandBuffer> command_buffers_;

    // Синхронизация
    std::vector<VkSemaphore> image_available_semaphores_;
    std::vector<VkSemaphore> render_finished_semaphores_;
    std::vector<VkFence> in_flight_fences_;

    // Uniform buffers
    std::vector<std::unique_ptr<uniform_buffer>> uniform_buffers_;
    std::vector<std::unique_ptr<uniform_buffer>> shadow_uniform_buffers_;
    VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptor_sets_;
    std::vector<VkDescriptorSet> shadow_descriptor_sets_;
    std::vector<VkDescriptorSet> shadow_map_descriptor_sets_;

    // Шейдеры
    std::unique_ptr<shader> vertex_shader_;
    std::unique_ptr<shader> fragment_shader_;
    std::unique_ptr<shader> shadow_vertex_shader_;
    std::unique_ptr<shader> shadow_fragment_shader_;

    // Состояние рендеринга
    uint32_t current_frame_       = 0;
    uint32_t current_image_index_ = 0;
    bool framebuffer_resized_     = false;
    vec4f clear_color_            = {0.1f, 0.1f, 0.1f, 1.0f};
    std::vector<VkFence> images_in_flight_;
    render_mode current_render_mode_ = render_mode::lit;

    // ImGui
    VkDescriptorPool imgui_descriptor_pool_ = VK_NULL_HANDLE;

    // Рендеринг примитивов
    VkPipelineLayout debug_pipeline_layout_ = VK_NULL_HANDLE;
    VkPipeline debug_pipeline_              = VK_NULL_HANDLE;
    std::unique_ptr<vertex_buffer> debug_vertex_buffer_;
    debug_primitives debug_primitives_;

    std::unique_ptr<shader> debug_vertex_shader_;
    std::unique_ptr<shader> debug_fragment_shader_;

    // Combined buffer pool для indirect drawing
    std::unique_ptr<combined_buffer_pool_type> combined_buffer_pool_;

    // Light buffer для point lights
    std::unique_ptr<light_buffer_type> light_buffer_;

    // Palette buffer для block colors
    const block_registry* block_registry_;
    std::unique_ptr<palette_buffer> palette_buffer_;

    // GPU frustum culling
    std::unique_ptr<cull_pipeline> cull_pipeline_;

    // Shadow map для directional light
    std::unique_ptr<shadow_map> shadow_map_;

    // Настройки directional light
    directional_light_settings directional_light_settings_;

    // Настройки тумана
    fog_settings fog_settings_;

    // Статистика
    mutable renderer_stats stats_;
    uint32 draw_call_count_ = 0;

    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
};

}  // namespace vw::gfx

#include "vw/gfx/render/renderer.inl.h"
#include "vw/gfx/render/renderer_palette.inl.h"
#include "vw/gfx/render/renderer_point_lights.inl.h"
#include "vw/gfx/render/renderer_shadow.inl.h"

#endif  // VW_GFX_RENDERER_H
