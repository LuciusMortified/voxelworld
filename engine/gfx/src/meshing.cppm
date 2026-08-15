export module vw.gfx:meshing;

import std;

import vw.core;
import vw.world;
import vulkan;

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

    // 64 KB of bit volume, reused by the worker across chunks and built once
    // per mesh. Heap rather than a member so the worker stack stays small.
    std::unique_ptr<vw::asset::chunk_occupancy> occupancy;
    bool occupancy_valid = false;

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

// Takes the mask cell rather than just the block id: the mask already holds the
// ambient occlusion for this quad, and every cell merged into it has the same
// one -- that is what the merge compares on.
// One layer reduced to bit rows: what is visible, what sits in front of the
// face (the plane ambient occlusion samples) and the layer itself (which is
// what top brightness samples). Empty says the layer holds no faces at all,
// which replaces the linear scan over the whole mask.
struct layer_rows {
    std::array<uint64, 64> visible{};
    std::array<uint64, 64> front{};
    std::array<uint64, 64> own{};

    // The plane in front lies outside the chunk, so every occlusion sample
    // around it leaves the chunk on two axes and reads as empty.
    bool front_outside = false;
};

[[nodiscard]] auto build_layer_rows(
    const vw::asset::model& mdl,
    const vw::asset::chunk_occupancy& occupancy,
    const face_axis_mapping& axes,
    int face_direction,
    int layer,
    layer_rows& out
) -> bool;

void emit_rect(
    mesh_generation_storage& storage,
    const face_axis_mapping& axes,
    int face_direction,
    int layer,
    int u_start,
    int v_start,
    int w,
    int h,
    const face_mask_cell& cell
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
    // Runs off the visibility bits instead of scanning the mask: a run starts
    // at countr_zero, a rectangle is cleared with one and-not per row, and the
    // mask is only consulted to compare keys of cells already known to be set.
    static void merge_and_emit_rects_bits(
        mesh_generation_storage& storage,
        const detail::face_axis_mapping& axes,
        int face_direction,
        int layer,
        detail::layer_rows& rows
    );

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
