#pragma once

#include <imgui.h>
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

#include "vw/common.h"
#include "vw/gfx/debug_primitive.h"

namespace vw::gfx {

class vertex_buffer;
class vulkan_context;
class window;
class camera;
class mesh;
class shader;
class uniform_buffer;
class world;

enum class render_mode { lit, wireframe };

struct uniform_buffer_object {
    alignas(16) float32 view[16];
    alignas(16) float32 projection[16];
    alignas(16) vec3f view_pos;
    alignas(16) vec3f light_pos;
    alignas(16) vec3f light_color;
};

struct push_constant_data {
    alignas(16) float32 model[16];
};

struct debug_push_constant_data {
    alignas(16) float32 viewProj[16];
};

class renderer {
public:
    renderer(vulkan_context& context, window& window);
    ~renderer();

    renderer(const renderer&)            = delete;
    renderer& operator=(const renderer&) = delete;

    void begin_frame();
    void end_frame();

    void render(world& world, camera& camera);

    void set_clear_color(float r, float g, float b, float a = 1.0f);
    void set_clear_color(vec4f color);
    void wait_idle() const;

    void handle_resize();

    void set_render_mode(render_mode mode);

    [[nodiscard]]
    render_mode get_render_mode() const {
        return current_render_mode_;
    }

    void draw_line(const vec3f& a, const vec3f& b, color col = colors::red);

    void
    draw_box(const vec3f& origin, const vec3f& size, color col = colors::red);

    void draw_grid(
        const vec3f& origin,
        float cel_size,
        int lines,
        color col = colors::gray
    );

private:
    void create_swapchain();
    void create_image_views();
    void create_depth_resources();
    void create_render_pass();
    void create_descriptor_set_layout();
    void create_graphics_pipeline();
    void create_wireframe_pipeline();
    void create_debug_pipeline();
    void create_framebuffers();
    void create_command_buffers();
    void create_sync_objects();
    void create_uniform_buffers();
    void create_descriptor_pool();
    void create_descriptor_sets();

    void init_imgui();
    void setup_imgui_style();
    void create_imgui_descriptor_pool();
    void cleanup_imgui();

    void cleanup_descriptor_pool();
    void cleanup_render_pass();
    void cleanup_descriptor_set_layout();
    void cleanup_pipelines();
    void cleanup_debug_pipeline();
    void cleanup_swapchain();
    void cleanup_depth_resources();
    void recreate_swapchain();

    void render_world(const world& world, const camera& camera) const;
    void update_uniform_buffer(const camera& camera) const;

    void render_debug_primitives(const camera& camera);
    void update_debug_vertex_buffer();

    void render_imgui() const;

    [[nodiscard]]
    VkFormat find_depth_format();

    [[nodiscard]]
    VkFormat find_supported_format(
        const std::vector<VkFormat>& candidates,
        VkImageTiling tiling,
        VkFormatFeatureFlags features
    );

    void create_image(
        VkImage& image,
        VkDeviceMemory& image_memory,
        VkExtent2D extent,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties
    );

    [[nodiscard]]
    VkImageView create_image_view(
        VkImage image,
        VkFormat format,
        VkImageAspectFlags aspect_flags
    );

    [[nodiscard]]
    uint32
    find_memory_type(uint32 typeFilter, VkMemoryPropertyFlags properties);

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

    // Depth
    VkImage depth_image_               = VK_NULL_HANDLE;
    VkDeviceMemory depth_image_memory_ = VK_NULL_HANDLE;
    VkImageView depth_image_view_      = VK_NULL_HANDLE;

    // Render pass и pipeline
    VkRenderPass render_pass_                    = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout_            = VK_NULL_HANDLE;
    VkPipeline graphics_pipeline_                = VK_NULL_HANDLE;
    VkPipeline wireframe_pipeline_               = VK_NULL_HANDLE;

    // Framebuffers и команды
    std::vector<VkFramebuffer> framebuffers_;
    std::vector<VkCommandBuffer> command_buffers_;

    // Синхронизация
    std::vector<VkSemaphore> image_available_semaphores_;
    std::vector<VkSemaphore> render_finished_semaphores_;
    std::vector<VkFence> in_flight_fences_;

    // Uniform buffers
    std::vector<std::unique_ptr<uniform_buffer>> uniform_buffers_;
    VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptor_sets_;

    // Шейдеры
    std::unique_ptr<shader> vertex_shader_;
    std::unique_ptr<shader> fragment_shader_;

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

    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
};

inline void renderer::draw_line(const vec3f& a, const vec3f& b, color col) {
    debug_primitives_.add_line(a, b, col);
}

inline void
renderer::draw_box(const vec3f& origin, const vec3f& size, color col) {
    debug_primitives_.add_box(origin, size, col);
}

inline void
renderer::draw_grid(const vec3f& origin, float cel_size, int lines, color col) {
    debug_primitives_.add_grid(origin, cel_size, lines, col);
}

}  // namespace vw::gfx