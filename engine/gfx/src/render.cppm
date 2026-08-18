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

    [[nodiscard]] auto get_timestamp_period() const -> float32;
    [[nodiscard]] auto get_timestamp_valid_bits() const -> uint32;

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

// ---- gpu timestamps
export namespace vw::gfx {

enum class gpu_stage : uint32 {
    frame,
    buffer_upload,
    compute_cull,
    shadow_pass,
    shadow_cascade_0,
    shadow_cascade_1,
    shadow_cascade_2,
    shadow_cascade_3,
    shadow_cascade_4,
    world_pass,
    world_geometry,
    world_debug,
    world_imgui,
    count,
};

inline constexpr uint32 gpu_stage_count = static_cast<uint32>(gpu_stage::count);

inline constexpr std::array<std::string_view, gpu_stage_count> gpu_stage_names{
    "gpu_frame",
    "gpu_buffer_upload",
    "gpu_compute_cull",
    "gpu_shadow_pass",
    "gpu_cascade_0",
    "gpu_cascade_1",
    "gpu_cascade_2",
    "gpu_cascade_3",
    "gpu_cascade_4",
    "gpu_world_pass",
    "gpu_world_geometry",
    "gpu_world_debug",
    "gpu_world_imgui",
};

struct gpu_timing_stats {
    std::array<float32, gpu_stage_count> ms{};
    bool supported = false;
};

// Every render_timing_stats field measures command recording, not execution:
// the frame is GPU-bound, so the CPU-side numbers say nothing about where the
// time goes. Timestamps are the only way to see it. Stages nest and the GPU
// overlaps them, so the parts do not have to add up to the whole.
class gpu_timer final {
public:
    gpu_timer(vulkan_context& context, uint32 frames_in_flight);
    ~gpu_timer();

    gpu_timer(const gpu_timer&)                    = delete;
    auto operator=(const gpu_timer&) -> gpu_timer& = delete;
    gpu_timer(gpu_timer&&)                         = delete;
    auto operator=(gpu_timer&&) -> gpu_timer&      = delete;

    auto reset(vk::CommandBuffer cmd, uint32 frame_index) -> void;
    auto begin(vk::CommandBuffer cmd, gpu_stage stage) const -> void;
    auto end(vk::CommandBuffer cmd, gpu_stage stage) const -> void;
    auto resolve(uint32 frame_index) -> void;

    [[nodiscard]] auto get_stats() const -> const gpu_timing_stats& { return stats_; }
    [[nodiscard]] auto is_supported() const -> bool { return supported_; }

private:
    [[nodiscard]] auto first_query_(uint32 frame_index) const -> uint32;

    vulkan_context* context_;
    vk::QueryPool pool_      = nullptr;
    uint32 frames_in_flight_ = 0;
    uint32 recording_frame_  = 0;
    float64 period_ns_       = 0.0;
    uint64 valid_mask_       = 0;
    bool supported_          = false;
    std::vector<uint8> frame_recorded_;
    std::vector<uint64> scratch_;
    gpu_timing_stats stats_{};
};

}  // namespace vw::gfx

// ---- from vw/gfx/render/shadow_map.h
export namespace vw::gfx {


class vulkan_context;
class camera;

// What the cascades are worth tuning by hand. Every one of these trades
// sharpness against how much of the frame the shadows cost, and the balance
// depends on how far the eye actually travels in a given game.
struct shadow_settings {
    // Off. Past the first cascade the edge staircased, and the staircase moved
    // whenever the camera did; no combination of the settings below made that
    // go away. Everything here stays as it was so the decision can be undone,
    // but nothing runs: no depth pass, no cascade culling, no filtering. The
    // matching switch is SHADOW_ENABLED in voxel.frag, and both have to agree.
    // What takes over the job is sky light -- docs/lighting.md.
    bool enabled = false;

    // Where the first cascade ends. Not the reach of the sharp zone -- the
    // first cascade's texel is roughly a thousandth of this, so pushing it out
    // dilutes that zone rather than extending it. What it buys is the ratio
    // between neighbours, which is what the staircase is made of. 32 out of a
    // thousand puts that ratio at 2.37: 1.6 screen pixels of staircase in the
    // first cascade and 2.3 at the near edge of every one after it, which is
    // what the cascade table in the test app prints.
    float32 first_split = 32.0f;

    // How far shadows are cast at all. Past it the world is lit flat.
    float32 distance = 1000.0f;

    // How far the sun may turn before a cascade is redrawn, in texels of
    // movement at the cascade rim. Smaller is smoother under a moving sun and
    // costs redraws. Set this low enough and it stops deciding anything: every
    // cascade is dirty every frame and the budget below becomes the whole
    // policy -- a round robin at one cascade per frame, which is a reasonable
    // thing to want, but it ties the shadow update rate to the frame rate
    // rather than to the speed of the sun.
    float32 turn_texels = 0.05f;

    // Cascades redrawn per frame. The whole point of the cascade cache is that
    // this is far below the cascade count.
    uint32 updates_per_frame = 1;

    // The penumbra, in texels of whichever cascade is being sampled.
    //
    // Half a texel is nearly nothing, and that is the point: width was worth
    // paying for while the staircase was five pixels and the far cascades
    // jumped a whole lattice at a time. With the ratio at 2.37 and every
    // cascade refreshing at the same rate, the edge holds still on its own, and
    // blurring it costs the contact between a face and its own shadow for
    // nothing in return.
    float32 filter_texels = 0.5f;

    // Depth offset along the surface normal, in texels, scaled by the filter
    // width. A kernel this tight reads almost the depth it was given, so it
    // needs almost no offset -- and offset it does not need is a shadow pulled
    // off its caster.
    float32 normal_bias = 0.0f;
    float32 slope_bias  = 0.5f;
};

class shadow_map {
public:
    // Five rather than four. What shows at a cascade boundary is the ratio
    // between neighbours, and with the splits in a geometric progression that
    // ratio is the same everywhere: over a given range, five cascades step by
    // the fourth root where four step by the third, and the staircase is that
    // ratio times 1.66 screen pixels. Over 24 to 1000 that was 3.47 against
    // 2.54, seven pixels against five. The price is a fifth layer of the shadow
    // array: 17 MB.
    static constexpr uint32 cascade_count = 5;

    explicit shadow_map(vulkan_context& context, uint32 size = 2048);

    [[nodiscard]] auto get_size() const -> uint32 { return size_; }
    ~shadow_map();

    shadow_map(const shadow_map&)            = delete;
    shadow_map& operator=(const shadow_map&) = delete;
    shadow_map(shadow_map&&)                 = delete;
    shadow_map& operator=(shadow_map&&)      = delete;

    void update(const camera& camera, const vec3f& light_direction);

    [[nodiscard]] auto get_settings() -> shadow_settings& { return settings_; }
    [[nodiscard]] auto get_settings() const -> const shadow_settings& { return settings_; }

    // A cascade holds world-space depth, so a map drawn on an earlier frame
    // stays correct as long as it still covers the segment it is asked about.
    // Each cascade is therefore padded and only redrawn once the camera walks
    // out of that padding, the light turns, or geometry inside it changes.
    void invalidate(const vw::spatial::aabb& bounds);
    void invalidate_all();
    [[nodiscard]] auto is_cascade_pending(uint32 cascade_index) const -> bool;
    [[nodiscard]] auto get_pending_count() const -> uint32;
    void clear_pending();

    [[nodiscard]] auto get_light_space_matrix(uint32 cascade_index) const -> mat4f;
    [[nodiscard]] auto get_light_space_matrices() const -> const std::array<mat4f, cascade_count>&;
    [[nodiscard]] auto get_cascade_splits() const -> const std::array<float, cascade_count>&;

    // World units per shadow texel, per cascade. What the fragment shader
    // measures its bias and its filter width in.
    [[nodiscard]] auto get_cascade_texel_sizes() const
        -> const std::array<float32, cascade_count>&;
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

    [[nodiscard]] auto select_cascades_(
        const std::array<vec3f, cascade_count>& centers,
        const std::array<float32, cascade_count>& radii
    ) -> uint32;

    void build_cascade_matrix_(
        uint32 cascade_index,
        const std::array<vec3f, 8>& corners,
        const vec3f& center,
        float32 radius,
        const vec3f& light_dir,
        float32 shadow_dist
    );

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

    std::array<float32, cascade_count> cascade_texel_sizes_ = {};
    std::array<vec3f, cascade_count> drawn_centers_ = {};
    std::array<float32, cascade_count> drawn_radii_ = {};
    // The direction each cascade was actually drawn with, not the direction of
    // the previous frame. Held per cascade because they are redrawn a couple at
    // a time, so they fall behind the sun by different amounts.
    std::array<vec3f, cascade_count> drawn_light_dirs_ = {};

    // Frames a cascade has been dirty without being picked. Breaks the tie
    // between cascades that all want redrawing equally badly.
    std::array<uint32, cascade_count> frames_waited_ = {};
    uint32 dirty_mask_   = 0;
    uint32 pending_mask_ = 0;

    uint32 size_;

    shadow_settings settings_{};

    // The splits the cascades were last built for. Changing either moves every
    // boundary, and a cascade cached against the old ones covers the wrong
    // slice.
    float32 built_first_split_ = 0.0f;
    float32 built_distance_    = 0.0f;

    // Coverage padding traded for update rate: the map is built this much
    // larger than the segment needs, so it stays valid while the camera drifts
    // that far. Costs resolution, buys frames with no shadow work at all.
    float32 cascade_padding_ratio_ = 0.18f;

    // Redraw is wanted slightly before the padding runs out. That narrow gap is
    // the only thing a cascade can wait in, and waiting is what keeps four of
    // them from falling due on the same frame.
    float32 cascade_trigger_ratio_ = 0.15f;

};

}  // namespace vw::gfx

// ---- from vw/gfx/render/cull_pipeline.h
export namespace vw::gfx {


class vulkan_context;

// The camera and every shadow cascade get culled in one dispatch, and every
// buffer the dispatch fills is sized by this: the plane block, the culled
// command buffer, the per-pass draw counts.
//
// Two copies of the number used to exist, both written out as literals, and a
// fifth cascade walked past the end of each in turn -- the plane fill ran six
// vec4 off a stack struct and died on the stack cookie, and the count buffer
// then handed the last cascade an offset one uint past its end.
inline constexpr uint32 cull_plane_count = (shadow_map::cascade_count + 1) * 6;

struct cull_frustum_ubo {
    vec4f planes[cull_plane_count];

    // Where the camera stands, for dropping the face directions that point away
    // from it. Only the camera pass uses it.
    vec4f eye;

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
        std::span<const vw::spatial::frustum> shadow_frustums,
        const vec3f& eye
    );

    // All the buffers at once. One barrier covers every count buffer instead
    // of one apiece: the barrier is a pipeline stall, and the pool grew from
    // nine buffers to fifteen when the size classes got finer.
    void dispatch(
        vk::CommandBuffer cmd,
        const std::vector<std::unique_ptr<combined_buffer>>& buffers,
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
