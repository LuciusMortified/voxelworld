export module vw.gfx:renderer;

import std;

import vw.core;
import vw.ecs;
import vw.world;
import vw.platform;
import :camera;
import :resource;
import :render;
import vulkan;

namespace vw::gfx {
using namespace ::vw::ecs;
using namespace ::vw::plat;
}

// ---- from vw/gfx/render/renderer.h
export namespace vw::gfx {


enum class render_mode : uint8 { lit, wireframe };

// Настройки directional light (для CPU)
struct directional_light_settings {
    vec3f direction{0.0f, -1.0f, 0.0f};
    vec3f color{1.f, 1.f, 1.f};
    float32 intensity{1.0f};
};

// The light a surface gets from everything that is not a light: the sky above
// and whatever the ground throws back. With no textures this is the only thing
// telling one face of a block from another wherever the sun does not reach, so
// it is a hemisphere rather than a constant -- a flat ambient makes an unlit
// voxel a single flat colour, which is what plastic looks like.
//
struct ambient_settings {
    vec3f sky{0.34f, 0.42f, 0.52f};
    vec3f ground{0.16f, 0.14f, 0.13f};
    float32 strength{0.85f};

    // How far down the ambient goes in a fully enclosed corner, and the curve
    // the raw occlusion travels to get there. The samples are integers from
    // zero to three, so the curve is the only say over where the middle of the
    // ramp sits: below one it spreads the shading out across the face, above
    // one it pulls it back into the corner.
    float32 ao_strength = 0.65f;
    float32 ao_curve    = 1.0f;

    // The other direction of the same factor: how much brighter a corner gets
    // where it sticks out instead of being shut in, and the curve that ramp
    // travels. Occlusion can only darken, so without this a top face under open
    // sky is exactly as bright as every other top face and the terrain reads
    // flat from the side the drops are on.
    float32 convex_strength = 0.35f;
    float32 convex_curve    = 1.0f;

    // What lights a place no sky reaches. A colour of its own rather than a
    // fraction of the sky above, so a sealed room stays the same brightness
    // while the day turns over its ceiling.
    vec3f cave{0.05f, 0.055f, 0.07f};

    // The curve sky visibility travels from sealed to open. Below one daylight
    // reaches further into a cave mouth, above one it stops at the entrance.
    float32 sky_curve = 1.0f;

    // The same visibility on its way to the sun, which needs a steeper curve of
    // its own. Sky light bends round corners because it is a flood; the sun
    // does not bend at all, so it has to be gone well before the sky is. At one
    // the two agree and daylight creeps fifteen voxels into a cave.
    float32 sun_curve = 2.0f;
};

// Not a setting -- a way of looking. Lighting is a product of the block's own
// colour and everything falling on it, and looking at the product to judge one
// factor is guesswork. Each of these shows one factor on its own.
enum class debug_view : uint32 {
    off = 0,
    ambient_occlusion,
    normals,
    sky_light,
    convexity,
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
    // Per cascade: x is where it ends in view depth, y is how much world one
    // of its shadow texels covers. Bias has to be measured in texels -- the
    // offset that is generous for the near cascade is a fraction of a texel for
    // the far one, and the far one is where the acne shows.
    //
    // A vec4 per cascade rather than a vec4 of splits and a vec4 of texel
    // sizes: there are five cascades now, and five scalars do not fit a vec4.
    alignas(16) vec4f cascades[shadow_map::cascade_count];

    // x: penumbra half-width in texels, y: normal offset in texels, z: how much
    // of that offset is added again per unit of slope.
    alignas(16) vec4f shadow_filter;

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

    // Hemisphere ambient: sky above, ground below, w of each unused.
    alignas(16) vec4f ambient_sky;
    alignas(16) vec4f ambient_ground;

    // x: how far down a fully enclosed corner goes, y: the occlusion curve.
    // z: how far up a corner that sticks out goes, w: the curve it travels.
    alignas(16) vec4f ao_params;

    // rgb: what lights a place no sky reaches.
    alignas(16) vec4f cave_ambient;

    // x: the curve sky visibility travels from sealed to open, for the ambient.
    // y: the same for the sun, which needs a steeper one -- see ambient_settings.
    alignas(16) vec4f sky_params;

    // Point lights count (для расширяемости)
    alignas(4) uint32 point_lights_count{0};

    alignas(4) uint32 debug_view{0};

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
    alignas(16) float32 matrix[16]{};
};

struct render_timing_stats {
    gpu_timing_stats gpu{};

    // Collecting finished meshes and requesting new ones. Was inside the
    // renderer total and nowhere else, which is how a 46 ms spike managed to
    // hide behind stages that all reported single digits.
    float32 mesh_sync_ms            = 0.0f;
    float32 shadow_map_update_ms    = 0.0f;
    float32 buffer_pool_update_ms   = 0.0f;
    float32 compute_cull_ms         = 0.0f;
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

class renderer final {
public:
    using world_type                = world;
    using combined_buffer_pool_type = combined_buffer_pool;
    using light_buffer_type         = light_buffer;

    renderer(vulkan_context& context, window& window, const block_registry& registry,
             uint32 mesh_workers = 0);
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

    [[nodiscard]] auto get_present_mode_name() const -> std::string_view;
    [[nodiscard]] static constexpr auto get_frames_in_flight() -> uint32 {
        return static_cast<uint32>(MAX_FRAMES_IN_FLIGHT);
    }

    [[nodiscard]] auto get_descriptor_pool() const -> vk::DescriptorPool {
        return descriptor_pool_;
    }
    [[nodiscard]] auto get_storage_descriptor_set_layout() const -> vk::DescriptorSetLayout {
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
    [[nodiscard]] auto get_ambient_settings() -> ambient_settings&;
    [[nodiscard]] auto get_shadow_settings() -> shadow_settings&;

    void set_debug_view(debug_view view);
    [[nodiscard]] auto get_debug_view() const -> debug_view;

    // Where each cascade ends and what one of its texels covers. Only worth
    // reading to show the cost of a shadow setting while it is being turned.
    [[nodiscard]] auto get_cascade_splits() const -> const std::array<float32, shadow_map::cascade_count>&;
    [[nodiscard]] auto get_cascade_texel_sizes() const -> const std::array<float32, shadow_map::cascade_count>&;

    [[nodiscard]] auto get_mesh_pool() -> mesh_pool& { return mesh_pool_; }
    [[nodiscard]] auto get_mesh_pool() const -> const mesh_pool& { return mesh_pool_; }

    // Drops chunks no open path reaches from the viewer. Off, the walk still
    // runs and reports what it would have hidden, so the two can be compared
    // in one build.
    void set_chunk_cull_enabled(bool enabled) {
        combined_buffer_pool_->set_chunk_cull_enabled(enabled);
    }

    void draw_colliders(world_type& w, color col = colors::green);

    // Получить ImTextureID для shadow map (для отображения в ImGui::Image)
    // В Vulkan это vk::DescriptorSet, приведенный к void*
    [[nodiscard]] void* get_shadow_map_texture_id(uint32 cascade_index = 0) const;

private:
    void create_swapchain();
    void create_image_views();
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
    void cleanup_depth_resources();
    void recreate_swapchain();

    void create_point_lights_descriptor_set_layout();
    void cleanup_point_lights_resources();

    void create_palette_descriptor_set_layout();
    void cleanup_palette_resources();

    void update_shadow_uniform_buffer() const;
    void render_shadow_pass(world_type& world, const camera& camera);

    void update_uniform_buffer(const camera& camera) const;
    void render_world_pass(world_type& world, const camera& camera);
    void render_world(world_type& world, const camera& camera);

    void sync_meshes_(world_type& world);

    void render_debug_primitives();
    void update_debug_vertex_buffer();

    void render_imgui() const;

    [[nodiscard]]
    vk::Format find_depth_format();

    [[nodiscard]]
    vk::Format find_supported_format(
        const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features
    );

    void create_image(
        vk::Image& image,
        vk::DeviceMemory& image_memory,
        vk::Extent2D extent,
        vk::Format format,
        vk::ImageTiling tiling,
        vk::ImageUsageFlags usage,
        vk::MemoryPropertyFlags properties,
        vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1
    );

    [[nodiscard]]
    vk::ImageView create_image_view(vk::Image image, vk::Format format, vk::ImageAspectFlags aspect_flags);

    [[nodiscard]]
    uint32 find_memory_type(uint32 typeFilter, vk::MemoryPropertyFlags properties);

    [[nodiscard]]
    static vk::SurfaceFormatKHR choose_swap_surface_format(
        const std::vector<vk::SurfaceFormatKHR>& available_formats
    );

    [[nodiscard]]
    static vk::PresentModeKHR choose_swap_present_mode(
        const std::vector<vk::PresentModeKHR>& available_present_modes
    );

    [[nodiscard]]
    vk::Extent2D choose_swap_extent(const vk::SurfaceCapabilitiesKHR& capabilities);

    vulkan_context* context_;
    window* window_;

    // Swapchain
    vk::SwapchainKHR swapchain_ = nullptr;
    std::vector<vk::Image> swapchain_images_;
    vk::Format swapchain_image_format_ = vk::Format::eUndefined;
    vk::Extent2D swapchain_extent_{};
    vk::PresentModeKHR present_mode_   = vk::PresentModeKHR::eFifo;
    std::vector<vk::ImageView> swapchain_image_views_;

    // Depth
    vk::Image depth_image_               = nullptr;
    vk::DeviceMemory depth_image_memory_ = nullptr;
    vk::ImageView depth_image_view_      = nullptr;

    // Render pass и pipeline
    vk::RenderPass render_pass_                                 = nullptr;
    vk::DescriptorSetLayout uniform_descriptor_set_layout_      = nullptr;
    vk::DescriptorSetLayout storage_descriptor_set_layout_      = nullptr;
    vk::DescriptorSetLayout shadow_descriptor_set_layout_       = nullptr;
    vk::DescriptorSetLayout point_lights_descriptor_set_layout_ = nullptr;
    vk::DescriptorSetLayout palette_descriptor_set_layout_      = nullptr;
    vk::PipelineLayout pipeline_layout_                         = nullptr;
    vk::Pipeline graphics_pipeline_                             = nullptr;
    vk::Pipeline wireframe_pipeline_                            = nullptr;
    vk::Pipeline shadow_pipeline_                               = nullptr;
    vk::PipelineLayout shadow_pipeline_layout_                  = nullptr;

    // Framebuffers и команды
    std::vector<vk::Framebuffer> framebuffers_;
    std::vector<vk::CommandBuffer> command_buffers_;

    // Синхронизация
    std::vector<vk::Semaphore> image_available_semaphores_;
    std::vector<vk::Semaphore> render_finished_semaphores_;
    std::vector<vk::Fence> in_flight_fences_;

    // Uniform buffers
    std::vector<std::unique_ptr<uniform_buffer>> uniform_buffers_;
    std::vector<std::unique_ptr<uniform_buffer>> shadow_uniform_buffers_;
    vk::DescriptorPool descriptor_pool_ = nullptr;
    std::vector<vk::DescriptorSet> descriptor_sets_;
    std::vector<vk::DescriptorSet> shadow_descriptor_sets_;
    std::vector<vk::DescriptorSet> shadow_map_descriptor_sets_;

    // Шейдеры
    std::unique_ptr<shader> vertex_shader_;
    std::unique_ptr<shader> fragment_shader_;
    std::unique_ptr<shader> shadow_vertex_shader_;
    std::unique_ptr<shader> shadow_fragment_shader_;

    // Состояние рендеринга
    uint32 current_frame_       = 0;
    uint64 frame_counter_       = 0;
    uint32 current_image_index_ = 0;
    bool framebuffer_resized_     = false;
    vec4f clear_color_            = {0.1f, 0.1f, 0.1f, 1.0f};
    std::vector<vk::Fence> images_in_flight_;
    render_mode current_render_mode_ = render_mode::lit;

    // ImGui
    vk::DescriptorPool imgui_descriptor_pool_ = nullptr;

    // Рендеринг примитивов
    vk::PipelineLayout debug_pipeline_layout_ = nullptr;
    vk::Pipeline debug_pipeline_              = nullptr;
    std::unique_ptr<vertex_buffer> debug_vertex_buffer_;
    debug_primitives debug_primitives_;

    std::unique_ptr<shader> debug_vertex_shader_;
    std::unique_ptr<shader> debug_fragment_shader_;

    // Mesh pool для генерации мешей
    mesh_pool mesh_pool_;
    std::unordered_set<entity> pending_mesh_entities_;

    // Declared before the pool so that it outlives every buffer it may hold
    deletion_queue deletion_queue_;

    // Combined buffer pool для indirect drawing
    std::unique_ptr<combined_buffer_pool_type> combined_buffer_pool_;

    // Light buffer для point lights
    std::unique_ptr<light_buffer_type> light_buffer_;

    // Palette buffer для block colors
    const block_registry* block_registry_;
    std::unique_ptr<palette_buffer> palette_buffer_;

    // GPU vw::spatial::frustum culling
    std::unique_ptr<cull_pipeline> cull_pipeline_;

    // Shadow map для directional light
    std::unique_ptr<shadow_map> shadow_map_;

    // GPU-время проходов кадра
    std::unique_ptr<gpu_timer> gpu_timer_;

    // Настройки directional light
    directional_light_settings directional_light_settings_;

    // Настройки тумана
    fog_settings fog_settings_;
    ambient_settings ambient_settings_;
    debug_view debug_view_ = debug_view::off;

    // Статистика
    mutable renderer_stats stats_;
    uint32 draw_call_count_ = 0;

    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
};

}  // namespace vw::gfx
