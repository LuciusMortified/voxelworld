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

// Для point lights (в SSBO)
struct point_light_data {
    alignas(16) vec4f position;
    alignas(16) vec4f color;
    alignas(4) float32 intensity;
    alignas(4) float32 range;
    alignas(4) float32 attenuation_constant;
    alignas(4) float32 attenuation_linear;
    alignas(4) float32 attenuation_quadratic;
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

    void update(world_type& world);

    [[nodiscard]] vk::DescriptorSet get_descriptor_set() const;
    [[nodiscard]] bool is_empty() const;
    [[nodiscard]] uint32 get_lights_count() const;

private:
    void expand_buffer_if_needed(uint32 required_count);
    void update_descriptor_set();

    static constexpr uint32 default_capacity_ = 64;

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

struct draw_command {
    uint32 index_count;
    uint32 instance_count;
    uint32 first_index;
    int32 vertex_offset;
    uint32 first_instance;
};

struct buffer_chunk_size {
    uint32 vertex_count;
    uint32 index_count;

    bool operator==(
        const buffer_chunk_size& rhs
    ) const {
        return vertex_count == rhs.vertex_count && index_count == rhs.index_count;
    }

    bool operator<(
        const buffer_chunk_size& rhs
    ) const {
        if (vertex_count != rhs.vertex_count) {
            return vertex_count < rhs.vertex_count;
        }
        return index_count < rhs.index_count;
    }
};

struct free_slot {
    uint32 vertex_offset;
    uint32 index_offset;
};

struct entity_allocation {
    uint32 instance_index;
    uint32 model_index;
};

struct mesh_allocation {
    uint32 vertex_offset;
    uint32 index_offset;
    uint32 vertex_count;
    uint32 index_count;
    uint32 generation;
    uint32 ref_count;
};

struct combined_buffer_stats {
    buffer_chunk_size chunk_size{};
    float32 vertex_load_min  = 0.0f;
    float32 vertex_load_max  = 0.0f;
    float32 vertex_load_avg  = 0.0f;
    float32 index_load_min   = 0.0f;
    float32 index_load_max   = 0.0f;
    float32 index_load_avg   = 0.0f;
    uint32 mesh_capacity     = 0;
    uint32 mesh_count        = 0;
    uint32 instance_capacity = 0;
    uint32 instance_count    = 0;
};

class combined_buffer {
public:
    static constexpr uint32 cull_pass_count = 5;

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

    [[nodiscard]] auto get_entity_allocation(entity ent) -> const entity_allocation&;
    [[nodiscard]] auto get_vertex_buffer() const -> vk::Buffer;
    [[nodiscard]] vk::Buffer get_index_buffer() const;
    [[nodiscard]] vk::Buffer get_instance_index_buffer() const;
    [[nodiscard]] vk::Buffer get_indirect_draw_buffer() const;
    [[nodiscard]] vk::Buffer get_aabb_buffer() const;
    [[nodiscard]] vk::Buffer get_model_matrix_buffer() const;
    [[nodiscard]] vk::Buffer get_normal_matrix_buffer() const;
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
    std::unique_ptr<device_vertex_buffer> vertex_buffer_;
    std::unique_ptr<device_index_buffer> index_buffer_;
    std::unique_ptr<device_storage_buffer> instance_index_buffer_;

    uint32 instance_capacity_{default_instance_capacity_};
    std::unique_ptr<device_storage_buffer> model_matrix_buffer_;
    std::unique_ptr<device_storage_buffer> normal_matrix_buffer_;
    std::unique_ptr<device_storage_buffer> indirect_draw_buffer_;
    std::unique_ptr<device_storage_buffer> aabb_buffer_;
    std::unique_ptr<device_storage_buffer> culled_indirect_buffer_;
    std::unique_ptr<device_storage_buffer> count_buffer_;

    std::unordered_map<entity, entity_allocation> entity_allocations_;
    std::unordered_map<uint32, mesh_allocation> mesh_allocations_;
    std::unordered_map<uint32, entity> instance_indexes_;
    std::vector<free_slot> free_slots_;
    uint32 vertex_used_{0};
    uint32 index_used_{0};

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

struct combined_buffer_pool_stats {
    float32 vertex_load_min  = 0.0f;
    float32 vertex_load_max  = 0.0f;
    float32 vertex_load_avg  = 0.0f;
    float32 index_load_min   = 0.0f;
    float32 index_load_max   = 0.0f;
    float32 index_load_avg   = 0.0f;
    uint32 mesh_capacity     = 0;
    uint32 mesh_count        = 0;
    uint32 instance_capacity = 0;
    uint32 instance_count    = 0;
    std::vector<combined_buffer_stats> buffers;
    buffer_pool_timing_stats timing;
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

    [[nodiscard]] static auto get_chunk_size_for_mesh(
        uint32 vertex_count, uint32 index_count
    ) -> buffer_chunk_size;

    [[nodiscard]] auto get_stats() const -> const combined_buffer_pool_stats&;

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

    vulkan_context* context_;
    deletion_queue* deletion_;
    staging_buffer staging_;
    std::vector<std::unique_ptr<combined_buffer>> buffers_;
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
