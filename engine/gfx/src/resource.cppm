export module vw.gfx:resource;

import std;

import vw.core;
import vw.ecs;
import vw.world;
import vw.platform;
import :camera;
import vulkan;

namespace vw::gfx {
using namespace ::vw::ecs;
using namespace ::vw::plat;
}

// ---- from vw/gfx/resource/shader.h
export namespace vw::gfx {
class vulkan_context;

enum class shader_type { VERTEX, FRAGMENT, COMPUTE };

class shader {
public:
    shader(vulkan_context& context, const std::string& path, shader_type type);
    ~shader();

    shader(const shader&)            = delete;
    shader& operator=(const shader&) = delete;

    shader(shader&&)            = delete;
    shader& operator=(shader&&) = delete;

    [[nodiscard]]
    vk::PipelineShaderStageCreateInfo get_stage_info() const;

    [[nodiscard]]
    vk::ShaderModule get_module() const {
        return shader_module_;
    }

private:
    [[nodiscard]]
    vk::ShaderModule create_shader_module(const std::vector<char>& code) const;

    static std::vector<char> read_file(const std::string& filename);

    vulkan_context* context_;

    vk::ShaderModule shader_module_;
    vk::ShaderStageFlagBits stage_;
};
}  // namespace vw::gfx

// ---- from vw/gfx/resource/buffer.h
export namespace vw::gfx {
class vulkan_context;

class buffer {
public:
    buffer(
        vulkan_context& context,
        vk::DeviceSize size,
        vk::BufferUsageFlags usage,
        vk::MemoryPropertyFlags properties
    );
    virtual ~buffer();

    buffer(const buffer&)            = delete;
    buffer& operator=(const buffer&) = delete;

    buffer(buffer&& other) noexcept;
    buffer& operator=(buffer&& other) noexcept;

    [[nodiscard]]
    vk::Buffer get_buffer() const {
        return buffer_;
    }

    [[nodiscard]]
    vk::DeviceMemory get_memory() const {
        return memory_;
    }

    [[nodiscard]]
    vk::DeviceSize get_size() const {
        return size_;
    }

    [[nodiscard]]
    void* map();

    void unmap();

    void copy_from(const void* data, vk::DeviceSize size, vk::DeviceSize offset = 0);

    void copy_to(void* data, vk::DeviceSize size, vk::DeviceSize offset = 0);

    template <typename T>
    void copy_from_struct(
        const T& data, vk::DeviceSize offset = 0
    ) {
        copy_from(&data, sizeof(T), offset);
    }

    template <typename T>
    void copy_from_vector(
        const std::vector<T>& data, vk::DeviceSize offset = 0
    ) {
        copy_from(data.data(), data.size() * sizeof(T), offset);
    }

    template <typename T>
    void copy_to_struct(
        T& dst, vk::DeviceSize offset = 0
    ) {
        copy_to(&dst, sizeof(T), offset);
    }

    void copy_to_buffer(
        buffer& dst, vk::DeviceSize size, vk::DeviceSize src_offset = 0, vk::DeviceSize dst_offset = 0
    );

protected:
    vk::DeviceSize size_;

private:
    void cleanup();

    [[nodiscard]]
    uint32 find_memory_type(uint32 type_filter, vk::MemoryPropertyFlags properties) const;

    vulkan_context* context_;

    vk::Buffer buffer_;
    vk::DeviceMemory memory_;
    void* mapped_memory_;
};

// Специализированные типы буферов
class vertex_buffer final : public buffer {
public:
    vertex_buffer(
        vulkan_context& context, vk::DeviceSize size
    )
        : buffer(
              context,
              size,
              vk::BufferUsageFlagBits::eVertexBuffer |     //
                  vk::BufferUsageFlagBits::eTransferSrc |   //
                  vk::BufferUsageFlagBits::eTransferDst,    //
              vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
          ) {}

    template <typename T>
    vertex_buffer(
        vulkan_context& context, const std::vector<T>& vertices
    )
        : buffer(
              context,
              sizeof(T) * vertices.size(),
              vk::BufferUsageFlagBits::eVertexBuffer |     //
                  vk::BufferUsageFlagBits::eTransferSrc |   //
                  vk::BufferUsageFlagBits::eTransferDst,    //
              vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
          ) {
        copy_from_vector(vertices);
    }
};

class index_buffer final : public buffer {
public:
    index_buffer(
        vulkan_context& context, vk::DeviceSize size
    )
        : buffer(
              context,
              size,
              vk::BufferUsageFlagBits::eIndexBuffer |      //
                  vk::BufferUsageFlagBits::eTransferSrc |   //
                  vk::BufferUsageFlagBits::eTransferDst,    //
              vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
          ) {}

    template <typename T>
    index_buffer(
        vulkan_context& context, const std::vector<T>& indices
    )
        : buffer(
              context,
              sizeof(T) * indices.size(),
              vk::BufferUsageFlagBits::eIndexBuffer |      //
                  vk::BufferUsageFlagBits::eTransferSrc |   //
                  vk::BufferUsageFlagBits::eTransferDst,    //
              vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
          ) {
        copy_from(indices.data(), size_);
    }
};

class uniform_buffer final : public buffer {
public:
    uniform_buffer(
        vulkan_context& context, vk::DeviceSize size
    )
        : buffer(
              context,
              size,
              vk::BufferUsageFlagBits::eUniformBuffer,
              vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
          ) {}

    template <typename T>
    uniform_buffer(
        vulkan_context& context, const T& data
    )
        : buffer(
              context,
              sizeof(T),
              vk::BufferUsageFlagBits::eUniformBuffer,
              vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
          ) {
        copy_from(&data, size_);
    }
};

class storage_buffer final : public buffer {
public:
    storage_buffer(
        vulkan_context& context, vk::DeviceSize size, vk::BufferUsageFlags additional_usage = {}
    )
        : buffer(
              context,
              size,
              vk::BufferUsageFlagBits::eStorageBuffer | additional_usage,
              vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
          ) {}

    template <typename T>
    storage_buffer(
        vulkan_context& context, const std::vector<T>& data, vk::BufferUsageFlags additional_usage = {}
    )
        : buffer(
              context,
              sizeof(T) * data.size(),
              vk::BufferUsageFlagBits::eStorageBuffer | additional_usage,
              vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
          ) {
        copy_from_vector(data);
    }
};
class device_vertex_buffer final : public buffer {
public:
    device_vertex_buffer(
        vulkan_context& context, vk::DeviceSize size
    )
        : buffer(
              context,
              size,
              vk::BufferUsageFlagBits::eVertexBuffer |      //
                  vk::BufferUsageFlagBits::eTransferSrc |    //
                  vk::BufferUsageFlagBits::eTransferDst,     //
              vk::MemoryPropertyFlagBits::eDeviceLocal
          ) {}
};

class device_index_buffer final : public buffer {
public:
    device_index_buffer(
        vulkan_context& context, vk::DeviceSize size
    )
        : buffer(
              context,
              size,
              vk::BufferUsageFlagBits::eIndexBuffer |        //
                  vk::BufferUsageFlagBits::eTransferSrc |    //
                  vk::BufferUsageFlagBits::eTransferDst,     //
              vk::MemoryPropertyFlagBits::eDeviceLocal
          ) {}
};

class device_storage_buffer final : public buffer {
public:
    device_storage_buffer(
        vulkan_context& context, vk::DeviceSize size, vk::BufferUsageFlags additional_usage = {}
    )
        : buffer(
              context,
              size,
              vk::BufferUsageFlagBits::eStorageBuffer |      //
                  vk::BufferUsageFlagBits::eTransferSrc |    //
                  vk::BufferUsageFlagBits::eTransferDst |    //
                  additional_usage,
              vk::MemoryPropertyFlagBits::eDeviceLocal
          ) {}
};

}  // namespace vw::gfx

// ---- from vw/gfx/resource/staging_buffer.h
export namespace vw::gfx {

class vulkan_context;

class staging_buffer {
public:
    explicit staging_buffer(
        vulkan_context& context,
        vk::DeviceSize frame_capacity  = 1 * 1024 * 1024,
        uint32 max_frames_in_flight = 2
    );
    ~staging_buffer();

    staging_buffer(const staging_buffer&)                    = delete;
    auto operator=(const staging_buffer&) -> staging_buffer& = delete;
    staging_buffer(staging_buffer&&)                          = delete;
    auto operator=(staging_buffer&&) -> staging_buffer&       = delete;

    void begin_frame();

    auto stage(const void* data, vk::DeviceSize size) -> vk::DeviceSize;

    template <typename T>
    auto stage_struct(const T& data) -> vk::DeviceSize {
        return stage(&data, sizeof(T));
    }

    template <typename T>
    auto stage_vector(const std::vector<T>& data) -> vk::DeviceSize {
        return stage(data.data(), data.size() * sizeof(T));
    }

    void copy_to(
        vk::Buffer dst, vk::DeviceSize dst_offset, vk::DeviceSize staging_offset, vk::DeviceSize size
    );

    void zero_region(vk::Buffer dst, vk::DeviceSize dst_offset, vk::DeviceSize size);

    [[nodiscard]] auto available() const -> vk::DeviceSize {
        return frame_end_offset_ - write_offset_;
    }

    void replace_buffer(vk::Buffer old_buf, vk::Buffer new_buf);

    void flush(vk::CommandBuffer cmd);

private:
    struct pending_copy {
        vk::Buffer src;
        vk::Buffer dst;
        vk::BufferCopy region;
    };

    vulkan_context* context_;
    vk::Buffer buffer_          = nullptr;
    vk::DeviceMemory memory_    = nullptr;
    void* mapped_             = nullptr;

    vk::DeviceSize frame_capacity_    = 0;
    uint32 max_frames_in_flight_    = 2;
    uint32 current_frame_index_     = 0;
    vk::DeviceSize write_offset_      = 0;
    vk::DeviceSize frame_end_offset_  = 0;

    std::vector<pending_copy> pending_copies_;
    std::vector<vk::BufferCopy> flush_regions_;
};

}  // namespace vw::gfx

// ---- from vw/gfx/resource/mesh.h
export namespace vw::gfx {

struct vertex {
    uint32 data0 = 0;
    uint32 data1 = 0;

    // data0: pos.x[6:0] | pos.y[13:7] | pos.z[20:14] | normal_id[23:21] | reserved[31:24]
    // data1: palette_index[7:0] | corners_dark[11:8] | corners_bright[15:12] | corner_id[17:16] | reserved[31:18]
    // corners_dark/bright: 4-bit binary mask, one bit per quad corner in winding order (same on all 4 verts of a quad)
    // corner_id: which winding-order corner (0..3) this vertex represents — used to pick UV in vertex shader

    vertex() = default;

    [[nodiscard]] static auto pack(
        int x, int y, int z, uint8 normal_id, block_id block_id,
        uint8 corners_dark, uint8 corners_bright, uint8 corner_id
    ) -> vertex;

    [[nodiscard]] static auto get_binding_descriptions()
        -> std::vector<vk::VertexInputBindingDescription>;

    [[nodiscard]] static auto get_attribute_descriptions()
        -> std::vector<vk::VertexInputAttributeDescription>;
};

struct mesh {
    std::vector<vertex> vertices;
    std::vector<uint32> indices;

    void release_data() {
        vertices = {};
        indices  = {};
    }
};

struct mesh_options {
    bool enable_top_brightness = false;
};

class simple_mesh_generator {
public:
    [[nodiscard]]
    static auto generate_mesh_data(
        const std::shared_ptr<vw::asset::model>& mdl,
        const block_registry& registry,
        mesh_options opts = {}
    ) -> mesh;

private:
    static void add_cube_face(
        std::vector<vertex>& vertices,
        std::vector<uint32>& indices,
        const std::shared_ptr<vw::asset::model>& mdl,
        int x,
        int y,
        int z,
        int face_direction,
        block_id voxel_id,
        const block_registry& registry,
        mesh_options opts
    );

    [[nodiscard]]
    static auto is_face_visible(
        const std::shared_ptr<vw::asset::model>& mdl, int x, int y, int z, int face_direction
    ) -> bool;
};

struct face_mask_cell {
    block_id voxel_id;
    uint8 corner_dark;
    uint8 corner_bright;

    [[nodiscard]]
    auto operator==(const face_mask_cell&) const -> bool = default;

    [[nodiscard]]
    auto is_empty() const -> bool {
        return voxel_id == blocks::air;
    }
};

struct mesh_generation_storage {
    std::vector<vertex> vertices;
    std::vector<uint32> indices;
    std::vector<face_mask_cell> mask;
    std::vector<bool> depth_has_pages;

    void clear() {
        vertices.clear();
        indices.clear();
    }
};

namespace detail {
struct face_axis_mapping {
    int width, height, depth;
    int face_direction;
    int32 voxel_scale;

    face_axis_mapping(const vw::asset::model& mdl, int face_dir);

    [[nodiscard]] auto to_model_coords(int u, int v, int layer) const -> std::tuple<int, int, int>;

    [[nodiscard]] auto to_local_min_max(int u, int v, int w, int h, int layer) const
        -> std::pair<vec3i, vec3i>;
};

[[nodiscard]] auto compute_corner_darkness(const vw::asset::model& mdl, int x, int y, int z, int face) -> uint8;

[[nodiscard]] auto is_face_visible(const vw::asset::model& mdl, int x, int y, int z, int face_direction)
    -> bool;

void build_face_mask(
    mesh_generation_storage& storage,
    const vw::asset::model& mdl,
    const face_axis_mapping& axes,
    int face_direction,
    int layer,
    mesh_options opts
);

void add_quad(
    std::vector<vertex>& vertices,
    std::vector<uint32>& indices,
    int face_direction,
    vec3i min_pos,
    vec3i max_pos,
    uint8 palette_index,
    uint8 corner_dark,
    uint8 corner_bright
);

void emit_rect(
    mesh_generation_storage& storage,
    const vw::asset::model& mdl,
    const face_axis_mapping& axes,
    int face_direction,
    int layer,
    int u_start,
    int v_start,
    int w,
    int h,
    uint8 voxel_id,
    const block_registry& registry,
    mesh_options opts
);
}  // namespace detail

class strip_mesh_generator {
public:
    [[nodiscard]]
    static auto generate_mesh_data(
        mesh_generation_storage& storage,
        const vw::asset::model& mdl,
        const block_registry& registry,
        mesh_options opts = {}
    ) -> mesh;

private:
    static void merge_and_emit_strips(
        mesh_generation_storage& storage,
        const vw::asset::model& mdl,
        const detail::face_axis_mapping& axes,
        int face_direction,
        int layer,
        const block_registry& registry,
        mesh_options opts
    );

    static void generate_face_quads(
        mesh_generation_storage& storage,
        const vw::asset::model& mdl,
        int face_direction,
        const block_registry& registry,
        mesh_options opts
    );
};

class greedy_mesh_generator {
public:
    [[nodiscard]]
    static auto generate_mesh_data(
        mesh_generation_storage& storage,
        const vw::asset::model& mdl,
        const block_registry& registry,
        mesh_options opts = {}
    ) -> mesh;

private:
    static void merge_and_emit_rects(
        mesh_generation_storage& storage,
        const vw::asset::model& mdl,
        const detail::face_axis_mapping& axes,
        int face_direction,
        int layer,
        const block_registry& registry,
        mesh_options opts
    );

    static void generate_face_quads(
        mesh_generation_storage& storage,
        const vw::asset::model& mdl,
        int face_direction,
        const block_registry& registry,
        mesh_options opts
    );
};
}  // namespace vw::gfx

// ---- from vw/gfx/resource/mesh_pool.h
export namespace vw::gfx {


class vulkan_context;

struct mesh_generation_task {
    vw::asset::model_identity identity;
    std::weak_ptr<vw::asset::model> model_ref;
    std::promise<mesh> promise;
    mesh_options opts;

    mesh_generation_task(
        vw::asset::model_identity identity,
        std::weak_ptr<vw::asset::model> model_ref,
        mesh_options opts
    )
        : identity(identity), model_ref(std::move(model_ref)), opts(opts) {}
};

class mesh_pool final {
public:
    explicit mesh_pool(vulkan_context& context, const block_registry& registry);
    ~mesh_pool();

    mesh_pool(const mesh_pool&)                    = delete;
    auto operator=(const mesh_pool&) -> mesh_pool& = delete;
    mesh_pool(mesh_pool&&)                         = delete;
    auto operator=(mesh_pool&&) -> mesh_pool&      = delete;

    void stop_gen_threads();
    [[nodiscard]] auto has(const vw::asset::model_identity& identity) const -> bool;
    [[nodiscard]] auto is_pending(const vw::asset::model_identity& identity) const -> bool;
    void request_mesh(const std::shared_ptr<vw::asset::model>& model_ptr, mesh_options opts = {});
    [[nodiscard]] auto get(const vw::asset::model_identity& identity) const -> std::shared_ptr<mesh>;
    void remove(const vw::asset::model_identity& identity);
    void evict(const vw::asset::model_identity& identity);
    void process_completed();
    [[nodiscard]] auto get_pending_count() const -> uint32;

private:
    void gen_thread_function();
    void sweep_orphaned_();

    vulkan_context* context_;
    const block_registry* registry_;
    std::unordered_map<vw::asset::model_identity, std::shared_ptr<mesh>> meshes_;
    std::unordered_map<vw::asset::model_identity, std::weak_ptr<vw::asset::model>> model_refs_;
    std::unordered_map<vw::asset::model_identity, std::future<mesh>> pending_meshes_;
    std::unordered_set<uint32> pending_indices_;

    std::vector<std::thread> gen_threads_;
    std::queue<std::unique_ptr<mesh_generation_task>> gen_queue_;
    std::mutex gen_mutex_;
    std::condition_variable gen_cv_;
    bool gen_running_ = true;
    uint32 sweep_counter_ = 0;
    static constexpr uint32 sweep_interval_ = 60;

};

}  // namespace vw::gfx

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
        staging_buffer& staging
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
    void expand_mesh_buffers_();
    void expand_instance_buffers_();
    void update_descriptor_set_();
    void update_compute_descriptor_set_();

    static constexpr uint32 default_mesh_capacity_     = 32;
    static constexpr uint32 default_instance_capacity_ = 64;

    vulkan_context* context_;
    staging_buffer* staging_;
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

private:
    auto get_or_create_buffer(const buffer_chunk_size& chunk_size) -> combined_buffer*;

    void process_destroyed_(world_type& world);
    void update_meshes_(world_type& world, const vec3f& camera_pos, mesh_pool& pool);
    void update_transforms_(world_type& world);

    vulkan_context* context_;
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
