#pragma once

#ifndef VW_GFX_MESH_H
#define VW_GFX_MESH_H

#include <vulkan/vulkan.h>

#include <array>
#include <memory>
#include <vector>

#include "vw/core/types.h"
#include "vw/core/vec3.h"
#include "vw/gfx/model/model.h"
#include "vw/gfx/resource/buffer.h"

namespace vw::gfx {
struct vertex {
    vec3f position;
    vec3f normal;
    uint32 color;
    float32 ao;

    vertex() : color(0), ao(1.0f) {}
    vertex(const vec3f& pos, const vec3f& norm, uint32 col, float32 ao_ = 1.0f)
        : position(pos), normal(norm), color(col), ao(ao_) {}

    [[nodiscard]] static auto get_binding_descriptions()
        -> std::vector<VkVertexInputBindingDescription>;

    [[nodiscard]] static auto get_attribute_descriptions()
        -> std::vector<VkVertexInputAttributeDescription>;
};

struct mesh {
    std::vector<vertex> vertices;
    std::vector<uint32> indices;
};

class simple_mesh_generator {
public:
    [[nodiscard]]
    static auto generate_mesh_data(const std::shared_ptr<model>& model) -> mesh;

private:
    static void add_cube_face(
        std::vector<vertex>& vertices,
        std::vector<uint32>& indices,
        const std::shared_ptr<model>& model,
        int x, int y, int z,
        int face_direction,
        color color
    );

    [[nodiscard]]
    static auto is_face_visible(
        const std::shared_ptr<model>& model, int x, int y, int z, int face_direction
    ) -> bool;
};

struct greedy_mesh_storage {
    std::vector<vertex> vertices;
    std::vector<uint32> indices;
    std::vector<color> mask;
    std::vector<bool> depth_has_pages;

    void clear() {
        vertices.clear();
        indices.clear();
    }
};

class greedy_mesh_generator {
public:
    [[nodiscard]]
    static auto generate_mesh_data(
        greedy_mesh_storage& storage, const model& mdl) -> mesh;

private:
    struct face_axis_mapping;

    static void build_face_mask(
        greedy_mesh_storage& storage,
        const model& mdl,
        const face_axis_mapping& axes,
        int face_direction,
        int layer
    );

    static void merge_and_emit_strips(
        greedy_mesh_storage& storage,
        const model& mdl,
        const face_axis_mapping& axes,
        int face_direction,
        int layer
    );

    static void generate_face_quads(
        greedy_mesh_storage& storage,
        const model& mdl,
        int face_direction
    );

    static void add_quad(
        std::vector<vertex>& vertices,
        std::vector<uint32>& indices,
        int face_direction,
        const vec3f& min_pos,
        const vec3f& max_pos,
        color color,
        const std::array<float32, 4>& ao
    );

    [[nodiscard]]
    static auto is_face_visible(
        const model& mdl, int x, int y, int z, int face_direction
    ) -> bool;

    [[nodiscard]]
    static auto compute_vertex_ao(
        const model& mdl, int x, int y, int z, int face, int su, int sv
    ) -> float32;
};
}  // namespace vw::gfx

#include "vw/gfx/resource/mesh.inl.h"

#endif  // VW_GFX_MESH_H
