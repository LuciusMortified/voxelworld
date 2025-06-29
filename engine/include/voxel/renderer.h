#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

#include <voxel/types.h>

namespace voxel {
    class vulkan_context;
    class window;
    class camera;
    class mesh;
    class shader;
    class uniform_buffer;
    class world;

    struct uniform_buffer_object {
        alignas(16) float view[16];
        alignas(16) float projection[16];
        alignas(16) vec3f view_pos;
        alignas(16) vec3f light_pos;
        alignas(16) vec3f light_color;
    };

    struct push_constant_data {
        alignas(16) float model[16];
    };

    class renderer {
    public:
        renderer(std::shared_ptr<vulkan_context> context, std::shared_ptr<window> window);
        ~renderer();

        // Запретить копирование
        renderer(const renderer&) = delete;
        renderer& operator=(const renderer&) = delete;

        void begin_frame();
        void end_frame();

        void render_mesh(std::shared_ptr<mesh> mesh, const vec3f& position, const vec3f& rotation = {}, const vec3f& scale = {1.0f, 1.0f, 1.0f});
        void render_world(const std::shared_ptr<world>& world, std::shared_ptr<camera> camera);

        void set_clear_color(const colorf& color);
        void set_clear_color(float r, float g, float b, float a = 1.0f);
        void wait_idle();

        // Обработка изменения размера окна
        void handle_resize();

    private:
        void create_swapchain();
        void create_image_views();
        void create_render_pass();
        void create_descriptor_set_layout();
        void create_graphics_pipeline();
        void create_framebuffers();
        void create_command_buffers();
        void create_sync_objects();
        void create_uniform_buffers();
        void create_descriptor_pool();
        void create_descriptor_sets();

        void cleanup_swapchain();
        void recreate_swapchain();

        void update_uniform_buffer(uint32_t current_image, const camera& camera);

        VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats);
        VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes);
        VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities);

        std::shared_ptr<vulkan_context> context_;
        std::shared_ptr<window> window_;

        // Swapchain
        VkSwapchainKHR swapchain_;
        std::vector<VkImage> swapchain_images_;
        VkFormat swapchain_image_format_;
        VkExtent2D swapchain_extent_;
        std::vector<VkImageView> swapchain_image_views_;

        // Render pass и pipeline
        VkRenderPass render_pass_;
        VkDescriptorSetLayout descriptor_set_layout_;
        VkPipelineLayout pipeline_layout_;
        VkPipeline graphics_pipeline_;

        // Framebuffers и команды
        std::vector<VkFramebuffer> framebuffers_;
        std::vector<VkCommandBuffer> command_buffers_;

        // Синхронизация
        std::vector<VkSemaphore> image_available_semaphores_;
        std::vector<VkSemaphore> render_finished_semaphores_;
        std::vector<VkFence> in_flight_fences_;

        // Uniform buffers
        std::vector<std::unique_ptr<uniform_buffer>> uniform_buffers_;
        VkDescriptorPool descriptor_pool_;
        std::vector<VkDescriptorSet> descriptor_sets_;

        // Шейдеры
        std::unique_ptr<shader> vertex_shader_;
        std::unique_ptr<shader> fragment_shader_;

        // Состояние рендеринга
        uint32_t current_frame_;
        uint32_t current_image_index_;
        bool framebuffer_resized_;
        colorf clear_color_;

        static const int MAX_FRAMES_IN_FLIGHT = 2;
    };
}