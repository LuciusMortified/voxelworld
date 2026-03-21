#pragma once

#ifndef VW_GFX_MESH_H
#define VW_GFX_MESH_H

#include <vulkan/vulkan.h>

#include <array>
#include <memory>
#include <vector>

#include "vw/core/block_registry.h"
#include "vw/core/types.h"
#include "vw/core/vec3.h"
#include "vw/gfx/model/model.h"
#include "vw/gfx/resource/buffer.h"

namespace vw::gfx {
struct vertex {
    uint32 data0 = 0;
    uint32 data1 = 0;

    // data0: pos.x[6:0] | pos.y[13:7] | pos.z[20:14] | normal_id[23:21] | ao[25:24] | reserved[31:26]
    // data1: palette_index[7:0] | reserved[31:8]

    vertex() = default;

    [[nodiscard]] static auto pack(
        int x, int y, int z, uint8 normal_id, uint8 ao, uint8 palette_index) -> vertex;

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
    static auto generate_mesh_data(
        const std::shared_ptr<model>& model, const block_registry& registry) -> mesh;

private:
    static void add_cube_face(
        std::vector<vertex>& vertices,
        std::vector<uint32>& indices,
        const std::shared_ptr<model>& model,
        int x, int y, int z,
        int face_direction,
        uint8 voxel_id,
        const block_registry& registry
    );

    [[nodiscard]]
    static auto is_face_visible(
        const std::shared_ptr<model>& model, int x, int y, int z, int face_direction
    ) -> bool;
};

struct face_mask_cell {
    uint8 voxel_id;
    uint8 ao_key;

    auto operator==(const face_mask_cell&) const -> bool = default;
    [[nodiscard]] auto is_empty() const -> bool { return voxel_id == 0; }
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

    [[nodiscard]] auto to_local_min_max(
        int u, int v, int w, int h, int layer
    ) const -> std::pair<vec3i, vec3i>;
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
    vec3i min_pos,
    vec3i max_pos,
    uint8 palette_index,
    const std::array<uint8, 4>& ao
);

void emit_rect(
    mesh_generation_storage& storage,
    const model& mdl,
    const face_axis_mapping& axes,
    int face_direction,
    int layer,
    int u_start, int v_start, int w, int h,
    uint8 voxel_id,
    const block_registry& registry
);
}  // namespace detail

class strip_mesh_generator {
public:
    [[nodiscard]]
    static auto generate_mesh_data(
        mesh_generation_storage& storage, const model& mdl,
        const block_registry& registry) -> mesh;

private:
    static void merge_and_emit_strips(
        mesh_generation_storage& storage,
        const model& mdl,
        const detail::face_axis_mapping& axes,
        int face_direction,
        int layer,
        const block_registry& registry
    );

    static void generate_face_quads(
        mesh_generation_storage& storage,
        const model& mdl,
        int face_direction,
        const block_registry& registry
    );
};

class greedy_mesh_generator {
public:
    [[nodiscard]]
    static auto generate_mesh_data(
        mesh_generation_storage& storage, const model& mdl,
        const block_registry& registry) -> mesh;

private:
    static void merge_and_emit_rects(
        mesh_generation_storage& storage,
        const model& mdl,
        const detail::face_axis_mapping& axes,
        int face_direction,
        int layer,
        const block_registry& registry
    );

    static void generate_face_quads(
        mesh_generation_storage& storage,
        const model& mdl,
        int face_direction,
        const block_registry& registry
    );
};
}  // namespace vw::gfx

#include "vw/gfx/resource/mesh.inl.h"

#endif  // VW_GFX_MESH_H
