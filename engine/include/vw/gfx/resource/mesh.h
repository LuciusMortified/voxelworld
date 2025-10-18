#pragma once

#ifndef VW_GFX_MESH_H
#define VW_GFX_MESH_H

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

#include "vw/core/model.h"
#include "vw/core/vec3.h"
#include "vw/core/types.h"
#include "vw/gfx/resource/buffer.h"

namespace vw::gfx {
class vulkan_context;

struct vertex {
    vec3f position;
    vec3f normal;
    uint32 color;

    vertex() : color(0) {}
    vertex(const vec3f& pos, const vec3f& norm, uint32 col)
        : position(pos), normal(norm), color(col) {}

    [[nodiscard]]
    static std::vector<VkVertexInputBindingDescription> get_binding_descriptions();

    [[nodiscard]]
    static std::vector<VkVertexInputAttributeDescription> get_attribute_descriptions();
};

// Структура для хранения данных меша без Vulkan буферов
struct mesh_data {
    std::vector<vertex> vertices;
    std::vector<uint32> indices;

    mesh_data() = default;
    mesh_data(const std::vector<vertex>& v, const std::vector<uint32>& i)
        : vertices(v), indices(i) {}
};

class mesh {
public:
    explicit mesh(vulkan_context& context);
    ~mesh() = default;

    mesh(const mesh&)            = delete;
    mesh& operator=(const mesh&) = delete;

    mesh(mesh&&)            = default;
    mesh& operator=(mesh&&) = delete;

    void set_vertices(const std::vector<vertex>& vertices);
    void set_indices(const std::vector<uint32>& indices);
    void set_mesh_data(const mesh_data& data);

    void bind(VkCommandBuffer command_buffer) const;
    void draw(VkCommandBuffer command_buffer) const;
    void draw_indexed(VkCommandBuffer command_buffer) const;

    [[nodiscard]]
    size_t get_vertex_count() const {
        return vertex_count_;
    }

    [[nodiscard]]
    size_t get_index_count() const {
        return index_count_;
    }

private:
    vulkan_context* context_;
    std::unique_ptr<vertex_buffer> vertex_buffer_;
    std::unique_ptr<index_buffer> index_buffer_;
    size_t vertex_count_;
    size_t index_count_;
};

class simple_mesh_generator {
public:
    [[nodiscard]]
    static mesh generate_from_model(vulkan_context& context, const std::shared_ptr<model>& model);

    [[nodiscard]]
    static mesh_data generate_mesh_data(const std::shared_ptr<model>& model);

private:
    static void add_cube_face(
        std::vector<vertex>& vertices,
        std::vector<uint32>& indices,
        const vec3f& position,
        int face_direction,
        color color
    );

    [[nodiscard]]
    static bool is_face_visible(
        const std::shared_ptr<model>& model,
        int x,
        int y,
        int z,
        int face_direction
    );
};

class greedy_mesh_generator {
public:
    [[nodiscard]]
    static mesh generate_from_model(vulkan_context& context, const std::shared_ptr<model>& model);

    [[nodiscard]]
    static mesh_data generate_mesh_data(const std::shared_ptr<model>& model);

private:
    static void generate_face_quads(
        std::vector<vertex>& vertices,
        std::vector<uint32>& indices,
        const std::shared_ptr<model>& model,
        int face_direction
    );
    static void add_quad(
        std::vector<vertex>& vertices,
        std::vector<uint32>& indices,
        const vec3f& min_pos,
        const vec3f& max_pos,
        int face_direction,
        color color
    );

    [[nodiscard]]
    static bool is_face_visible(
        const std::shared_ptr<model>& model,
        int x,
        int y,
        int z,
        int face_direction
    );
};
}  // namespace vw::gfx

#endif  // VW_GFX_MESH_H
