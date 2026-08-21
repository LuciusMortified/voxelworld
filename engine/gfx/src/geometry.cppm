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

class light_buffer {
public:
    using world_type = world;

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
    void update(world_type& world, const spatial::frustum& frustum, const vec3f& eye);

    [[nodiscard]] vk::DescriptorSet get_descriptor_set() const;
    [[nodiscard]] bool is_empty() const;
    [[nodiscard]] uint32 get_lights_count() const;

private:
    void expand_buffer_if_needed(uint32 required_count);
    void update_descriptor_set();

    static constexpr uint32 default_capacity_ = 64;

    // Where the per-pixel loop stops being free. Two dozen visible sources
    // is the number docs/lighting.md names as the point past which this
    // wants clustering rather than a longer list.
    static constexpr std::size_t max_visible_ = 24;

    vulkan_context* context_;
    deletion_queue* deletion_;
    uint32 capacity_;
    std::unique_ptr<storage_buffer> lights_buffer_;

    vk::DescriptorSet descriptor_set_              = nullptr;
    vk::DescriptorPool descriptor_pool_            = nullptr;
    vk::DescriptorSetLayout descriptor_set_layout_ = nullptr;

    uint32 lights_count_ = 0;
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
    // milliseconds. See docs/frame-time-baseline.md -- a single false opening
    // floods a cave network that is connected almost everywhere, and the cell
    // size needed to avoid that grows with the size of the world.
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
