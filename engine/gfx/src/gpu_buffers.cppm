export module vw.gfx:gpu_buffers;

import std;

import vw.core;
import vulkan;

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

namespace vw::gfx {

// Owns GPU buffers that have been replaced while frames referencing them may
// still be in flight, and destroys them once the owning frame is known
// complete. Scoped RAII cannot express this: the point of release is a fence,
// not a scope exit.
class deletion_queue final {
public:
    deletion_queue()  = default;
    ~deletion_queue() = default;

    deletion_queue(const deletion_queue&)                    = delete;
    auto operator=(const deletion_queue&) -> deletion_queue& = delete;
    deletion_queue(deletion_queue&&)                         = delete;
    auto operator=(deletion_queue&&) -> deletion_queue&      = delete;

    auto set_frame(uint64 frame) -> void;
    auto retire(std::unique_ptr<buffer> victim) -> void;
    auto collect(uint64 completed_frame) -> void;

    [[nodiscard]] auto pending_count() const -> uint32;

private:
    struct entry {
        uint64 frame;
        std::unique_ptr<buffer> victim;
    };

    std::vector<entry> entries_;
    uint64 current_frame_ = 0;
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

    // Device-to-device copy recorded into the same frame command buffer. Flush
    // orders these ahead of the staging writes, which is what buffer growth
    // needs: carry the old contents over first, then apply this frame's writes.
    void copy_buffer(
        vk::Buffer src, vk::DeviceSize src_offset,
        vk::Buffer dst, vk::DeviceSize dst_offset,
        vk::DeviceSize size
    );


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
