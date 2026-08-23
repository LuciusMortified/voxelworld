export module vw.gfx:geometry;

import std;

import vw.core;
import vw.ecs;
import vw.world;
import :camera;
import :gpu_buffers;
import :meshing;
import :mesh_pool;
import vulkan;

namespace vw::gfx {
using namespace ::vw::ecs;
}

// ---- from vw/gfx/resource/palette_buffer.h
export namespace vw::gfx {

class vulkan_context;

class palette_buffer {
public:
    palette_buffer(
        vulkan_context& context,
        vk::DescriptorPool descriptor_pool,
        vk::DescriptorSetLayout descriptor_set_layout,
        const block_registry& registry
    );
    ~palette_buffer();

    [[nodiscard]] auto get_descriptor_set() const -> vk::DescriptorSet;

private:
    vulkan_context* context_;
    std::unique_ptr<storage_buffer> buffer_;
    vk::DescriptorSet descriptor_set_              = nullptr;
    vk::DescriptorPool descriptor_pool_            = nullptr;
    vk::DescriptorSetLayout descriptor_set_layout_ = nullptr;
};

}  // namespace vw::gfx

// ---- from vw/gfx/resource/light_buffer.h
export namespace vw::gfx {


class vulkan_context;

// Для point lights (в SSBO). Two numbers and not five: the falloff is the one
// the baked channel uses, and it wants a peak and a reach. See light_component.
struct point_light_data {
    alignas(16) vec4f position;
    alignas(16) vec4f color;
    alignas(4) float32 intensity;
    alignas(4) float32 range;
};

// How far under a body its patch is still worth drawing, in fall heights.
//
// The disc used to have no such number at all -- it narrows as one over one
// plus the height and never reaches zero. That was fine while eight of them
// rode in the frame uniform and every ground pixel walked all eight; it is not
// fine for a cull, because nothing unbounded can be put in a list of the places
// it reaches.
//
// Three, because by then the disc is a quarter as wide and a sixteenth as dark:
// for the bodies in the tree that is under a third of a voxel across and about
// nine levels of an eight-bit channel. The last quarter of the reach is faded
// out in the shader so the end is a fade and not an edge.
constexpr float32 blob_reach_falls = 3.0F;

// One patch of ground darkened under one body.
struct blob_data {
    // xyz where the body's feet are, w the radius of the disc under it.
    alignas(16) vec4f position_radius;

    // x the height over which rising tells, y how dark the middle is, z how
    // tall the body is -- which is what says where the ground under it stops
    // and the body itself starts -- w how far down it still reaches.
    alignas(16) vec4f params;

    // The column of ground this darkens, as the two ends of a capsule: xyz the
    // top, w the radius, then xyz the bottom. Worked out on the CPU and carried
    // rather than derived twice -- the cull and the reference have to agree on
    // it exactly, and the cheapest way to agree is to be handed the same
    // numbers.
    //
    // A capsule and not a ball because the column is tall and thin: measured on
    // two hundred bodies, a ball around it claimed 27308 clusters a frame where
    // the column itself claims under six thousand.
    alignas(16) vec4f cull_a;
    alignas(16) vec4f cull_b;
};

class light_buffer {
public:
    using world_type = world;

    // One buffer per frame in flight, the same ring the frame uniforms use.
    // A single host-visible buffer was rewritten by the CPU while the frame
    // before it could still be reading -- invisible while a fragment shader is
    // the only reader, and not harmless at all once a list of indices points
    // into it.
    static constexpr uint32 max_frames_in_flight = 2;

    explicit light_buffer(
        vulkan_context& context,
        deletion_queue& deletion,
        vk::DescriptorPool descriptor_pool,
        vk::DescriptorSetLayout descriptor_set_layout
    );
    ~light_buffer();

    // Rebuilt every frame rather than when the component changes. It used to
    // be the latter and that was wrong: moving an entity does not touch its
    // light_component, so a torch carried by a player uploaded its position
    // once and then lit the spot it started from for ever. Sixty-four lights
    // are two kilobytes; the early-out was saving nothing worth a bug.
    //
    // Culled here against the frustum, because the fragment shader walks every
    // light in the buffer for every pixel and has no way to skip one.
    auto update(
        world_type& world, const spatial::frustum& frustum, const vec3f& eye, uint32 frame_index
    ) -> void;

    [[nodiscard]] auto get_descriptor_set(uint32 frame_index) const -> vk::DescriptorSet;
    [[nodiscard]] auto is_empty() const -> bool;
    [[nodiscard]] auto get_lights_count() const -> uint32;

    // What went into the buffer this frame, in the order the shader indexes it.
    // The cull needs it to say what it was asked, and keeping it costs the one
    // allocation the vector was making anyway.
    [[nodiscard]] auto get_lights() const -> std::span<const point_light_data> {
        return lights_;
    }

    // A cap that keeps the nearest and drops the rest. no_cap is the default:
    // with the froxel lists the fragment no longer walks the buffer, so the
    // number that used to protect the per-pixel loop protects nothing and only
    // drops sources. --bench-visible puts one back to price the naive path.
    static constexpr uint32 no_cap = std::numeric_limits<uint32>::max();

    [[nodiscard]] auto get_max_visible() -> uint32& {
        return max_visible_;
    }

private:
    auto expand_buffer_if_needed_(uint32 frame_index, uint32 required_count) -> void;
    auto update_descriptor_set_(uint32 frame_index) -> void;

    static constexpr uint32 default_capacity_ = 64;

    uint32 max_visible_ = no_cap;

    vulkan_context* context_;
    deletion_queue* deletion_;
    std::vector<point_light_data> lights_;
    std::array<uint32, max_frames_in_flight> capacities_{};
    std::array<std::unique_ptr<storage_buffer>, max_frames_in_flight> lights_buffers_;
    std::array<vk::DescriptorSet, max_frames_in_flight> descriptor_sets_{};

    vk::DescriptorPool descriptor_pool_            = nullptr;
    vk::DescriptorSetLayout descriptor_set_layout_ = nullptr;

    uint32 lights_count_ = 0;
};

struct light_cull_ubo {
    alignas(16) float32 view[16]{};
    alignas(16) vec4f cluster_params{};
    alignas(16) vec4f cluster_extent{};
    alignas(16) vec4f screen_dims{};
    alignas(16) vec4<uint32> cull_dims{};
};

// What the compute pass writes and the fragment reads: counts and indices, and
// nothing else. No bounding volumes, no prefix sum, no separate pass to build
// the grid.
//
// vw::spatial::cluster_grid is the geometry and scatter_slice the reference
// that shaders/light_cull.comp translates; this is the Vulkan around them.
// The patches under the bodies, in the same descriptor set as the sources and
// on the same ring. Binding 3 of set 3; the sets belong to light_buffer, which
// is why they arrive from outside rather than being allocated here.
class blob_buffer {
public:
    using world_type = world;

    static constexpr uint32 max_frames_in_flight = 2;

    blob_buffer(
        vulkan_context& context,
        deletion_queue& deletion,
        std::span<const vk::DescriptorSet> sets
    );

    // No cap and no nearest-first. There used to be eight slots and an
    // nth_element deciding which eight, which meant the ninth body in a crowd
    // stood on nothing at all; with a list per cluster the ground pixel walks
    // the two or three patches over it rather than every body in the frame.
    auto update(world_type& world, uint32 frame_index) -> void;

    [[nodiscard]] auto get_blobs() const -> std::span<const blob_data> {
        return blobs_;
    }

    [[nodiscard]] auto get_count() const -> uint32 {
        return static_cast<uint32>(blobs_.size());
    }

private:
    auto expand_buffer_if_needed_(uint32 frame_index, uint32 required_count) -> void;
    auto write_binding_(uint32 frame_index) -> void;

    static constexpr uint32 default_capacity_ = 32;

    vulkan_context* context_;
    deletion_queue* deletion_;
    std::vector<blob_data> blobs_;
    std::array<uint32, max_frames_in_flight> capacities_{};
    std::array<std::unique_ptr<storage_buffer>, max_frames_in_flight> buffers_;
    std::array<vk::DescriptorSet, max_frames_in_flight> sets_{};
};

// off costs nothing and is what a normal frame runs. counts is the cheap half:
// enough for how full the grid is and how much overflowed. full adds the lists
// themselves, which is megabytes a frame and only worth it under --verify-lights.
enum class cluster_readback_level : uint8 {
    off,
    counts,
    full,
};

// Two lists over one grid: the sources that light a cluster and the bodies that
// shade it. The same pass builds both, from the same code, because the question
// -- which clusters does this ball touch -- is the same one twice.
enum class cull_list : uint32 {
    sources = 0,
    blobs   = 1,
};

inline constexpr uint32 cull_list_count = 2;

// What one frame's cull was asked and what it answered. Carried back a whole
// ring later, when the fence for that frame has already been waited on, so that
// reading it never stalls anything and never races the GPU writing it.
struct cluster_readback {
    cull_list kind = cull_list::sources;
    spatial::cluster_grid grid{};
    uint32 cap = 0;

    // The shader's input and not the scene's: view space, depth positive
    // forwards, exactly what the compute pass built for itself. A source is a
    // ball and a body's patch of shade a column, and both are capsules, so
    // `kind` says what the entries mean rather than which list holds them.
    std::vector<spatial::view_capsule> columns;

    // cluster_count + 1 long, the last entry being the overflow tally.
    std::vector<uint32> counts;

    // Empty at the counts level. cluster_count * cap long at the full one.
    std::vector<uint32> indices;
};

class light_grid {
public:
    static constexpr uint32 max_frames_in_flight = 2;

    // The sets are light_buffer's. Bindings 1, 2, 4 and 5 of set 3 belong here;
    // 0 is light_buffer's own and 3 is blob_buffer's, all written into the same
    // set so that the compute pass and the fragment reach all six with one bind.
    light_grid(
        vulkan_context& context,
        deletion_queue& deletion,
        vk::DescriptorPool descriptor_pool,
        vk::DescriptorSetLayout light_set_layout,
        std::span<const vk::DescriptorSet> light_sets
    );
    ~light_grid();

    light_grid(const light_grid&)                    = delete;
    auto operator=(const light_grid&) -> light_grid& = delete;
    light_grid(light_grid&&)                         = delete;
    auto operator=(light_grid&&) -> light_grid&      = delete;

    // Held rather than acted on. Buffers are rebuilt inside dispatch, for the
    // frame being recorded, whose fence begin_frame has already waited on --
    // rewriting a descriptor set another frame is still reading is the one way
    // to get this wrong.
    auto set_grid(const spatial::cluster_grid& grid, uint32 cap, uint32 blob_cap) -> void;

    auto dispatch(
        vk::CommandBuffer cmd,
        vk::DescriptorSet light_set,
        const mat4f& view,
        std::span<const point_light_data> lights,
        std::span<const blob_data> blobs,
        uint32 frame_index
    ) -> void;

    auto set_readback(cluster_readback_level level) -> void;

    // The frame that finished, or nothing. Whatever is not taken is dropped
    // when the next one lands: a checker that fell behind should check a recent
    // frame rather than a queue of old ones.
    [[nodiscard]] auto take_readback(cull_list kind) -> std::optional<cluster_readback>;

    [[nodiscard]] auto get_cluster_count() const -> uint32;
    [[nodiscard]] auto get_cap() const -> uint32;

private:
    // One list's buffers for one frame in flight, plus what that frame was
    // asked, waiting for its answer to come back.
    struct list_frame {
        std::unique_ptr<device_storage_buffer> counts;
        std::unique_ptr<device_storage_buffer> indices;
        std::unique_ptr<storage_buffer> counts_host;
        std::unique_ptr<storage_buffer> indices_host;
        cluster_readback pending{};
        bool pending_valid = false;
    };

    auto create_pipeline_() -> void;
    auto create_params_ubos_() -> void;
    auto rebuild_frame_(uint32 frame_index) -> void;
    auto rebuild_list_(cull_list kind, uint32 frame_index) -> void;
    auto rebuild_mirrors_(cull_list kind, uint32 frame_index) -> void;
    auto harvest_(cull_list kind, uint32 frame_index) -> void;
    auto record_(
        vk::CommandBuffer cmd, vk::DescriptorSet light_set, cull_list kind, uint32 sphere_count,
        uint32 frame_index
    ) -> void;
    auto write_params_(cull_list kind, const mat4f& view, uint32 sphere_count, uint32 frame_index)
        -> void;
    auto snapshot_(cull_list kind, uint32 frame_index) -> cluster_readback&;

    [[nodiscard]] auto cap_of(cull_list kind) const -> uint32;

    [[nodiscard]] auto list_(cull_list kind, uint32 frame_index) -> list_frame& {
        return lists_[static_cast<uint32>(kind)][frame_index];
    }

    [[nodiscard]] static auto params_slot_(cull_list kind, uint32 frame_index) -> uint32 {
        return (frame_index * cull_list_count) + static_cast<uint32>(kind);
    }

    // Counts, plus one past them for the tally of assignments that did not fit.
    [[nodiscard]] auto counts_size_() const -> vk::DeviceSize;
    [[nodiscard]] auto indices_size_(cull_list kind) const -> vk::DeviceSize;

    vulkan_context* context_;
    deletion_queue* deletion_;
    vk::DescriptorPool descriptor_pool_ = nullptr;

    std::unique_ptr<shader> compute_shader_;
    vk::Pipeline compute_pipeline_                        = nullptr;
    vk::PipelineLayout compute_pipeline_layout_           = nullptr;
    vk::DescriptorSetLayout params_descriptor_set_layout_ = nullptr;
    vk::DescriptorSetLayout light_set_layout_             = nullptr;

    static constexpr uint32 params_slots_ = max_frames_in_flight * cull_list_count;

    std::array<std::unique_ptr<uniform_buffer>, params_slots_> params_ubos_;
    std::array<vk::DescriptorSet, params_slots_> params_descriptor_sets_{};

    std::array<vk::DescriptorSet, max_frames_in_flight> light_sets_{};
    std::array<std::array<list_frame, max_frames_in_flight>, cull_list_count> lists_{};

    cluster_readback_level readback_ = cluster_readback_level::off;
    std::array<std::optional<cluster_readback>, cull_list_count> ready_{};

    // Bumped whenever the grid changes shape; a frame whose buffers carry an
    // older one rebuilds them the next time it is recorded.
    std::array<uint64, max_frames_in_flight> built_{};
    uint64 generation_ = 1;

    spatial::cluster_grid grid_{};
    uint32 cap_ = 32;

    // A cluster holding more than a few bodies is a crowd standing on one
    // another, and the sixteenth patch over a pixel is not one anybody can
    // pick out. Half a megabyte a frame rather than the sources' two and a
    // half.
    uint32 blob_cap_      = 16;
    uint32 cluster_count_ = 0;
};

}  // namespace vw::gfx

// ---- from vw/gfx/resource/combined_buffer.h
export namespace vw::gfx {




class vulkan_context;

// Indexed against one index buffer shared by the whole pool, whose contents are
// the same pattern for every mesh: 0,1,2,2,3,0 per quad. vertex_offset is what
// puts gl_VertexIndex on this mesh, so the shader finds its record at
// gl_VertexIndex / 4 and the corner at gl_VertexIndex % 4.
//
// Six vertices unrolled from gl_VertexIndex with no index buffer was tried
// first and is a third more vertex shading -- four invocations a quad become
// six, with no post-transform reuse. It cost 38 % of the world pass. The shared
// buffer is a megabyte and a half for the whole engine, against the 24 bytes a
// quad the per-mesh index buffers used to cost.
struct draw_command {
    uint32 index_count;
    uint32 instance_count;
    uint32 first_index;
    int32 vertex_offset;
    uint32 first_instance;
};

struct buffer_chunk_size {
    uint32 quad_count;

    bool operator==(
        const buffer_chunk_size& rhs
    ) const {
        return quad_count == rhs.quad_count;
    }

    bool operator<(
        const buffer_chunk_size& rhs
    ) const {
        return quad_count < rhs.quad_count;
    }
};

struct free_slot {
    uint32 quad_offset;
};

struct entity_allocation {
    uint32 instance_index;
    uint32 model_index;
};

struct mesh_allocation {
    uint32 quad_offset;
    uint32 quad_count;
    uint32 generation;
    uint32 ref_count;

    // The quads of each face direction, in the order the mesher emits them.
    // One draw command per direction, so the culling shader can leave out the
    // ones pointing away from the viewer.
    std::array<uint32, 6> face_counts{};
};

struct combined_buffer_stats {
    buffer_chunk_size chunk_size{};
    float32 quad_load_min    = 0.0f;
    float32 quad_load_max    = 0.0f;
    float32 quad_load_avg    = 0.0f;
    uint32 mesh_capacity     = 0;
    uint32 mesh_count        = 0;
    uint32 instance_capacity = 0;
    uint32 instance_count    = 0;
};

class combined_buffer {
public:
    // The camera and every shadow cascade are culled in one dispatch, and the
    // buffers it fills are sized by this. It has to equal the cascade count
    // plus one; :geometry cannot see :render without closing a cycle in the
    // import graph, so cull_pipeline.cpp asserts the two agree.
    static constexpr uint32 cull_pass_count = 6;

    // One draw command per face direction of a mesh.
    static constexpr uint32 faces_per_mesh = 6;

    explicit combined_buffer(
        vulkan_context& context,
        const buffer_chunk_size& chunk_size,
        vk::DescriptorPool descriptor_pool,
        vk::DescriptorSetLayout descriptor_set_layout,
        vk::DescriptorSetLayout compute_descriptor_set_layout,
        staging_buffer& staging,
        deletion_queue& deletion
    );
    ~combined_buffer();

    combined_buffer(const combined_buffer&)            = delete;
    combined_buffer& operator=(const combined_buffer&) = delete;

    combined_buffer(combined_buffer&&) noexcept            = default;
    combined_buffer& operator=(combined_buffer&&) noexcept = default;

    void allocate(
        entity e, vw::asset::model_identity model_id, const mesh& mesh_data,
        const mat4f& transform_matrix, const vw::spatial::aabb& bounds
    );
    void allocate_mesh(vw::asset::model_identity model_id, const mesh& mesh_data);
    void write_mesh(vw::asset::model_identity model_id, const mesh& mesh_data);
    void write_transform(entity ent, const mat4f& transform_matrix, const vw::spatial::aabb& bounds);
    auto free(entity ent) -> std::optional<entity>;

    // One flag per instance, read by the culling shader before the frustum
    // test. The span covers instances from zero; anything past its end is
    // treated as visible, so a buffer nobody has an opinion about draws in
    // full.
    void write_visibility(std::span<const uint32> flags);

    [[nodiscard]] auto get_entity_allocation(entity ent) -> const entity_allocation&;
    [[nodiscard]] auto get_quad_buffer() const -> vk::Buffer;
    [[nodiscard]] vk::Buffer get_instance_index_buffer() const;
    [[nodiscard]] vk::Buffer get_indirect_draw_buffer() const;
    [[nodiscard]] vk::Buffer get_aabb_buffer() const;
    [[nodiscard]] vk::Buffer get_model_matrix_buffer() const;
    [[nodiscard]] vk::Buffer get_normal_matrix_buffer() const;
    // Instances in the buffer, and the ceiling on commands a culling pass can
    // produce out of them -- six a mesh, one per face direction.
    [[nodiscard]] uint32 get_instance_count() const;
    [[nodiscard]] uint32 get_draw_command_count() const;
    [[nodiscard]] vk::Buffer get_culled_indirect_buffer() const;
    [[nodiscard]] vk::Buffer get_count_buffer() const;
    [[nodiscard]] bool is_empty() const;
    [[nodiscard]] const combined_buffer_stats& get_stats() const;
    [[nodiscard]] vk::DescriptorSet get_descriptor_set() const {
        return descriptor_set_;
    }
    [[nodiscard]] vk::DescriptorSet get_compute_descriptor_set() const {
        return compute_descriptor_set_;
    }

private:
    void write_draw_command_(uint32 instance_index, const mesh_allocation& mesh_alloc);
    void expand_mesh_buffers_();
    void expand_instance_buffers_();
    void update_descriptor_set_();
    void update_compute_descriptor_set_();

    static constexpr uint32 default_mesh_capacity_     = 32;
    static constexpr uint32 default_instance_capacity_ = 64;

    vulkan_context* context_;
    staging_buffer* staging_;
    deletion_queue* deletion_;
    buffer_chunk_size chunk_size_;

    uint32 mesh_capacity_{default_mesh_capacity_};
    std::unique_ptr<device_storage_buffer> quad_buffer_;
    std::unique_ptr<device_storage_buffer> instance_index_buffer_;

    uint32 instance_capacity_{default_instance_capacity_};
    std::unique_ptr<device_storage_buffer> model_matrix_buffer_;
    std::unique_ptr<device_storage_buffer> normal_matrix_buffer_;
    std::unique_ptr<device_storage_buffer> indirect_draw_buffer_;
    std::unique_ptr<device_storage_buffer> aabb_buffer_;
    std::unique_ptr<device_storage_buffer> culled_indirect_buffer_;
    std::unique_ptr<device_storage_buffer> count_buffer_;
    std::unique_ptr<device_storage_buffer> visibility_buffer_;

    std::unordered_map<entity, entity_allocation> entity_allocations_;
    std::unordered_map<uint32, mesh_allocation> mesh_allocations_;
    std::unordered_map<uint32, entity> instance_indexes_;
    std::vector<free_slot> free_slots_;
    uint32 quad_used_{0};

    vk::DescriptorSet descriptor_set_                       = nullptr;
    vk::DescriptorPool descriptor_pool_                     = nullptr;
    vk::DescriptorSetLayout descriptor_set_layout_          = nullptr;
    vk::DescriptorSet compute_descriptor_set_               = nullptr;
    vk::DescriptorSetLayout compute_descriptor_set_layout_  = nullptr;

    mutable combined_buffer_stats stats_;
};

}  // namespace vw::gfx

// ---- from vw/gfx/resource/combined_buffer_pool.h
export namespace vw::gfx {


class vulkan_context;

struct entity_buffer_info {
    buffer_chunk_size chunk_size;
    size_t buffer_index;
    vw::spatial::aabb bounds{};
};

struct buffer_pool_timing_stats {
    float32 destroyed_ms     = 0.0f;
    float32 meshes_ms        = 0.0f;
    float32 transforms_ms    = 0.0f;
    float32 staging_flush_ms = 0.0f;
};

struct chunk_cull_stats {
    uint32 chunks  = 0;
    uint32 visible = 0;
    float32 walk_ms = 0.0f;

    // How far the walk ranged and what it walked through. Cells with no chunk
    // count as open air, so a large `visited_empty` means the walk is getting
    // around the world rather than through it.
    uint32 visited       = 0;
    uint32 visited_empty = 0;
    uint32 sealed        = 0;
    uint32 known_links   = 0;
    uint32 merged        = 0;
    uint32 max_pockets   = 0;
};

struct combined_buffer_pool_stats {
    float32 quad_load_min    = 0.0f;
    float32 quad_load_max    = 0.0f;
    float32 quad_load_avg    = 0.0f;
    uint32 mesh_capacity     = 0;
    uint32 mesh_count        = 0;
    uint32 instance_capacity = 0;
    uint32 instance_count    = 0;

    // Entities whose upload did not fit the frame's staging budget and were put
    // off to the next one. Stuck above zero means the ring is being eaten faster
    // than it refills, and a mesh waiting there is geometry that is not on
    // screen yet -- which reads as a hole in the world rather than as a stall.
    uint32 mesh_pending     = 0;
    uint32 transform_pending = 0;

    std::vector<combined_buffer_stats> buffers;
    buffer_pool_timing_stats timing;
    chunk_cull_stats chunk_cull;
};

class combined_buffer_pool {
public:
    using world_type = world;

    explicit combined_buffer_pool(
        vulkan_context& context,
        deletion_queue& deletion,
        vk::DescriptorPool descriptor_pool,
        vk::DescriptorSetLayout descriptor_set_layout,
        vk::DescriptorSetLayout compute_descriptor_set_layout = nullptr
    );
    ~combined_buffer_pool() = default;

    combined_buffer_pool(const combined_buffer_pool&)            = delete;
    auto operator=(const combined_buffer_pool&) -> combined_buffer_pool& = delete;
    combined_buffer_pool(combined_buffer_pool&&)                 = delete;
    auto operator=(combined_buffer_pool&&) -> combined_buffer_pool&      = delete;

    void update(
        world_type& world,
        const camera& camera,
        vk::CommandBuffer cmd,
        mesh_pool& pool
    );

    [[nodiscard]] auto get_buffers() const -> const std::vector<std::unique_ptr<combined_buffer>>&;

    // The quad index pattern, shared by every buffer and every mesh in them.
    [[nodiscard]] auto get_index_buffer() const -> vk::Buffer;

    [[nodiscard]] static auto get_chunk_size_for_mesh(
        uint32 quad_count
    ) -> buffer_chunk_size;

    [[nodiscard]] auto get_stats() const -> const combined_buffer_pool_stats&;

    // Off by default: on the current world the walk hides nothing and costs
    // milliseconds. A single false opening floods a cave network that is
    // connected almost everywhere, and the cell size needed to avoid that grows
    // with the size of the world. Kept for sound, navigation and streaming,
    // where the answer wanted is exact rather than conservative.
    void set_chunk_cull_enabled(bool enabled) {
        chunk_cull_enabled_ = enabled;
    }

    [[nodiscard]] auto is_chunk_cull_enabled() const -> bool {
        return chunk_cull_enabled_;
    }

    // Where geometry moved, appeared or went away this frame. Consumed by the
    // shadow map, which only redraws the cascades those volumes touch.
    [[nodiscard]] auto get_touched_bounds() const -> std::span<const vw::spatial::aabb> {
        return touched_bounds_;
    }

private:
    auto get_or_create_buffer(const buffer_chunk_size& chunk_size) -> combined_buffer*;

    void process_destroyed_(world_type& world);
    void update_meshes_(world_type& world, const vec3f& camera_pos, mesh_pool& pool);
    void update_transforms_(world_type& world);
    void update_chunk_visibility_(world_type& world, const vec3f& camera_pos);
    void evict_uploaded_(world_type& world, mesh_pool& pool);

    vulkan_context* context_;
    deletion_queue* deletion_;
    staging_buffer staging_;
    void ensure_index_pattern_(uint32 quads);

    std::vector<std::unique_ptr<combined_buffer>> buffers_;
    std::unique_ptr<device_index_buffer> index_buffer_;
    std::unique_ptr<index_buffer> index_upload_;
    uint32 index_quads_ = 0;
    std::unordered_map<entity, entity_buffer_info> entity_buffer_infos_;
    std::map<buffer_chunk_size, size_t> chunk_size_to_buffer_index_;

    vk::DescriptorPool descriptor_pool_                     = nullptr;
    vk::DescriptorSetLayout descriptor_set_layout_          = nullptr;
    vk::DescriptorSetLayout compute_descriptor_set_layout_  = nullptr;

    std::vector<entity> entities_to_process_;
    std::vector<entity> mesh_pending_entities_;
    std::vector<entity> transform_pending_entities_;
    std::vector<entity> merge_buffer_;
    std::vector<vw::spatial::aabb> touched_bounds_;
    std::vector<std::pair<float32, entity>> sort_keys_;

    // Models whose geometry reached the GPU this frame, and the models entities
    // still queued are waiting on. The first set minus the second is the CPU
    // copy that may be dropped -- see evict_uploaded_.
    std::vector<vw::asset::model_identity> uploaded_models_;
    std::unordered_set<vw::asset::model_identity> awaited_models_;

    bool chunk_cull_enabled_ = false;
    std::vector<std::vector<uint32>> visibility_flags_;

    // Connectivity for every chunk that has been meshed, including the ones
    // with no geometry at all. Solid rock produces an empty mesh and never
    // reaches a buffer, but it is exactly what the walk has to stop against,
    // so this cannot live alongside the instances.
    std::unordered_map<entity, vw::asset::chunk_links> chunk_links_;

    // Top loaded chunk per column, which is what separates sky from a gap
    // inside the world.
    std::unordered_map<vec2i, int32> column_top_;

    mutable combined_buffer_pool_stats stats_;
};

}  // namespace vw::gfx

// ---- from vw/gfx/debug/debug_primitive.h
export namespace vw::gfx {

class vulkan_context;

struct debug_vertex {
    vec3f pos;
    color col;

    explicit debug_vertex(
        vec3f pos, color col = colors::red
    )
        : pos(pos), col(col) {}

    [[nodiscard]]
    static auto get_binding_descriptions() -> std::vector<vk::VertexInputBindingDescription>;

    [[nodiscard]]
    static auto get_attribute_descriptions() -> std::vector<vk::VertexInputAttributeDescription>;
};

class debug_primitives {
public:
    void add_line(const vec3f& begin, const vec3f& end, color clr = colors::red);

    void add_box(const mat4f& matrix, const vec3f& size, color clr = colors::red);
    void add_box(const transform& transform, const vec3f& size, color clr = colors::red);
    void add_box(const vec3f& pos, const vec3f& size, color clr = colors::red);

    void add_grid(
        const mat4f& matrix, float cell_size, int cols, int rows, color clr = colors::red
    );
    void add_grid(
        const transform& transform, float cell_size, int cols, int rows, color clr = colors::red
    );
    void add_grid(const vec3f& pos, float cell_size, int cols, int rows, color clr = colors::red);

    [[nodiscard]]
    auto get_vertices() const -> const std::vector<debug_vertex>&;

    [[nodiscard]]
    auto is_empty() const -> bool;

    void clear();

private:
    std::vector<debug_vertex> vertices_;
};

}  // namespace vw::gfx
