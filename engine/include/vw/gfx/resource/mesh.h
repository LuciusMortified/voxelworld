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

struct face_mask_cell {
    color col;
    uint8 ao_key;

    auto operator==(const face_mask_cell&) const -> bool = default;
    [[nodiscard]] auto is_empty() const -> bool { return col == colors::empty; }
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

    face_axis_mapping(const model& mdl, int face_dir);

    [[nodiscard]] auto to_model_coords(
        int u, int v, int layer
    ) const -> std::tuple<int, int, int>;

    [[nodiscard]] auto to_world_min_max(
        int u, int v, int w, int h, int layer
    ) const -> std::pair<vec3f, vec3f>;
};

[[nodiscard]] auto compute_vertex_ao_float(
    const model& mdl, int x, int y, int z, int face, int su, int sv
) -> float32;

[[nodiscard]] auto compute_vertex_ao_int(
    const model& mdl, int x, int y, int z, int face, int su, int sv
) -> int;

[[nodiscard]] auto compute_ao_key(
    const model& mdl, int x, int y, int z, int face
) -> uint8;

[[nodiscard]] auto is_face_visible(
    const model& mdl, int x, int y, int z, int face_direction
) -> bool;

void build_face_mask(
    mesh_generation_storage& storage,
    const model& mdl,
    const face_axis_mapping& axes,
    int face_direction,
    int layer
);

void add_quad(
    std::vector<vertex>& vertices,
    std::vector<uint32>& indices,
    int face_direction,
    const vec3f& min_pos,
    const vec3f& max_pos,
    color color,
    const std::array<float32, 4>& ao
);

void emit_rect(
    mesh_generation_storage& storage,
    const model& mdl,
    const face_axis_mapping& axes,
    int face_direction,
    int layer,
    int u_start, int v_start, int w, int h,
    color col
);
}  // namespace detail

class strip_mesh_generator {
public:
    [[nodiscard]]
    static auto generate_mesh_data(
        mesh_generation_storage& storage, const model& mdl) -> mesh;

private:
    static void merge_and_emit_strips(
        mesh_generation_storage& storage,
        const model& mdl,
        const detail::face_axis_mapping& axes,
        int face_direction,
        int layer
    );

    static void generate_face_quads(
        mesh_generation_storage& storage,
        const model& mdl,
        int face_direction
    );
};

class greedy_mesh_generator {
public:
    [[nodiscard]]
    static auto generate_mesh_data(
        mesh_generation_storage& storage, const model& mdl) -> mesh;

private:
    static void merge_and_emit_rects(
        mesh_generation_storage& storage,
        const model& mdl,
        const detail::face_axis_mapping& axes,
        int face_direction,
        int layer
    );

    static void generate_face_quads(
        mesh_generation_storage& storage,
        const model& mdl,
        int face_direction
    );
};
}  // namespace vw::gfx

#include "vw/gfx/resource/mesh.inl.h"

#endif  // VW_GFX_MESH_H
