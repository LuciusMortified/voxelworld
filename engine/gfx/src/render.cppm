export module vw.gfx:render;

import std;

import vw.core;
import vw.ecs;
import vw.world;
import :camera;
import vw.platform;
import :resource;
import vulkan;

namespace vw::gfx {
using namespace ::vw::ecs;
using namespace ::vw::plat;
}

// ---- from vw/gfx/render/vulkan_context.h
export namespace vw::gfx {


struct queue_family_indices {
    std::optional<uint32> graphics_family;
    std::optional<uint32> present_family;

    [[nodiscard]]
    auto is_complete() const -> bool {
        return graphics_family.has_value() && present_family.has_value();
    }
};

struct swapchain_support_details {
    vk::SurfaceCapabilitiesKHR capabilities{};
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> present_modes;
};

class vulkan_context final {
public:
    explicit vulkan_context(window& window);
    ~vulkan_context();

    vulkan_context(const vulkan_context&)                    = delete;
    auto operator=(const vulkan_context&) -> vulkan_context& = delete;

    [[nodiscard]] auto get_instance() const -> vk::Instance;
    [[nodiscard]] auto get_device() const -> vk::Device;
    [[nodiscard]] auto get_physical_device() const -> vk::PhysicalDevice;
    [[nodiscard]] auto get_graphics_queue() const -> vk::Queue;
    [[nodiscard]] auto get_present_queue() const -> vk::Queue;
    [[nodiscard]] auto get_surface() const -> vk::SurfaceKHR;
    [[nodiscard]] auto get_command_pool() const -> vk::CommandPool;
    [[nodiscard]] auto get_queue_families() const -> queue_family_indices;
    [[nodiscard]] auto query_swapchain_support_() const -> swapchain_support_details;

private:
    auto create_instance_() -> void;
    auto create_surface_() -> void;
    auto pick_physical_device_() -> void;
    auto create_logical_device_() -> void;
    auto create_command_pool_() -> void;

    [[nodiscard]] auto is_device_suitable_(vk::PhysicalDevice device) -> bool;
    [[nodiscard]] auto find_queue_families_(vk::PhysicalDevice device) -> queue_family_indices;
    [[nodiscard]] auto check_device_extension_support_(vk::PhysicalDevice device) -> bool;
    [[nodiscard]] auto query_swapchain_support_(vk::PhysicalDevice device) const
        -> swapchain_support_details;

    window* window_;

    vk::Instance instance_;
    vk::SurfaceKHR surface_;
    vk::PhysicalDevice physical_device_;
    vk::Device device_;
    vk::Queue graphics_queue_;
    vk::Queue present_queue_;
    vk::CommandPool command_pool_;
    queue_family_indices queue_families_;

    std::vector<const char*> device_extensions_;

#ifndef NDEBUG
    vk::DebugUtilsMessengerEXT debug_messenger_;

    auto setup_debug_messenger_() -> void;
#endif
};
}  // namespace vw::gfx

// ---- from vw/gfx/render/shadow_map.h
export namespace vw::gfx {


class vulkan_context;
class camera;

class shadow_map {
public:
    static constexpr uint32 cascade_count = 4;

    explicit shadow_map(vulkan_context& context, uint32 size = 2048);

    [[nodiscard]] auto get_size() const -> uint32 { return size_; }
    ~shadow_map();

    shadow_map(const shadow_map&)            = delete;
    shadow_map& operator=(const shadow_map&) = delete;
    shadow_map(shadow_map&&)                 = delete;
    shadow_map& operator=(shadow_map&&)      = delete;

    void update(const camera& camera, const vec3f& light_direction);

    [[nodiscard]] auto get_light_space_matrix(uint32 cascade_index) const -> mat4f;
    [[nodiscard]] auto get_light_space_matrices() const -> const std::array<mat4f, cascade_count>&;
    [[nodiscard]] auto get_cascade_splits() const -> const std::array<float, cascade_count>&;
    [[nodiscard]] auto get_cascade_frustums() const -> const std::array<vw::spatial::frustum, cascade_count>&;
    [[nodiscard]] auto get_image() const -> vk::Image;
    [[nodiscard]] auto get_image_view(uint32 cascade_index) const -> vk::ImageView;
    [[nodiscard]] auto get_array_image_view() const -> vk::ImageView;
    [[nodiscard]] auto get_sampler() const -> vk::Sampler;
    [[nodiscard]] auto get_debug_sampler() const -> vk::Sampler;
    [[nodiscard]] auto get_framebuffer(uint32 cascade_index) const -> vk::Framebuffer;
    [[nodiscard]] auto get_render_pass() const -> vk::RenderPass;

private:
    void create_shadow_map_image();
    void create_sampler();
    void create_framebuffers();
    void create_render_pass();
    void cleanup();

    vulkan_context* context_;

    vk::Image shadow_image_                                              = nullptr;
    vk::DeviceMemory shadow_image_memory_                                = nullptr;
    vk::ImageView shadow_array_image_view_                               = nullptr;
    std::array<vk::ImageView, cascade_count> shadow_cascade_image_views_ = {};
    vk::Sampler shadow_sampler_                                          = nullptr;
    vk::Sampler debug_sampler_                                           = nullptr;
    std::array<vk::Framebuffer, cascade_count> shadow_framebuffers_      = {};
    vk::RenderPass shadow_render_pass_                                   = nullptr;

    std::array<mat4f, cascade_count> light_space_matrices_ = {};
    std::array<vw::spatial::frustum, cascade_count> cascade_frustums_   = {};
    std::array<float, cascade_count> cascade_splits_       = {};

    uint32 size_;
    float split_lambda_ = 0.5f;
    float shadow_far_   = 1000.f;
};

}  // namespace vw::gfx

// ---- from vw/gfx/render/cull_pipeline.h
export namespace vw::gfx {


class vulkan_context;

struct cull_frustum_ubo {
    vec4f planes[30];
    uint32 pass_count;
    uint32 pad[3];
};

class cull_pipeline {
public:
    static constexpr uint32 max_frames_in_flight = 2;

    explicit cull_pipeline(
        vulkan_context& context,
        vk::DescriptorPool descriptor_pool
    );
    ~cull_pipeline();

    cull_pipeline(const cull_pipeline&)            = delete;
    auto operator=(const cull_pipeline&) -> cull_pipeline& = delete;
    cull_pipeline(cull_pipeline&&)                 = delete;
    auto operator=(cull_pipeline&&) -> cull_pipeline&      = delete;

    void update_frustums(
        uint32 frame_index,
        const vw::spatial::frustum& view_frustum,
        std::span<const vw::spatial::frustum> shadow_frustums
    );

    void dispatch(
        vk::CommandBuffer cmd,
        combined_buffer& buffer,
        uint32 frame_index
    );

    [[nodiscard]] auto get_buffer_descriptor_set_layout() const -> vk::DescriptorSetLayout {
        return buffer_descriptor_set_layout_;
    }

private:
    void create_descriptor_set_layouts_();
    void create_pipeline_();
    void create_frustum_ubos_();

    vulkan_context* context_;
    vk::DescriptorPool descriptor_pool_ = nullptr;

    std::unique_ptr<shader> compute_shader_;
    vk::Pipeline compute_pipeline_                      = nullptr;
    vk::PipelineLayout compute_pipeline_layout_         = nullptr;
    vk::DescriptorSetLayout frustum_descriptor_set_layout_ = nullptr;
    vk::DescriptorSetLayout buffer_descriptor_set_layout_  = nullptr;

    std::array<std::unique_ptr<uniform_buffer>, max_frames_in_flight> frustum_ubos_;
    std::array<vk::DescriptorSet, max_frames_in_flight> frustum_descriptor_sets_{};
};

}  // namespace vw::gfx
