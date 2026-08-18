export module vw.gfx:meshing;

import std;

import vw.core;
import vw.world;
import vulkan;

// ---- from vw/gfx/resource/mesh.h
export namespace vw::gfx {

// One greedy rectangle, eight bytes. There is no vertex buffer and no index
// buffer: the draw asks for six vertices per quad and the vertex shader picks
// the corner out of this record by gl_VertexIndex. Four vertices of eight bytes
// plus six indices of four came to fifty-six.
//
// data0: min.x[6:0] | min.y[13:7] | min.z[20:14] | normal_id[23:21]
//      | corners_ao[31:24]
// data1: max.x[6:0] | max.y[13:7] | max.z[20:14] | palette_index[28:21]
//
// The two corners are the box the rectangle spans; along the face axis they are
// equal. Seven bits each, so a model is at most 128 voxels a side -- the same
// ceiling the packed vertex had.
//
// corners_ao: two bits per corner in winding order, 0 open to 3 fully enclosed.
// It used to be one bit per corner plus one more mask that brightened whatever
// nothing stood over -- so a corner touched by a single diagonal block came out
// exactly as dark as the inside of a right angle, and the gradation the sampler
// computes was thrown away at the last step. The brightening was standing in
// for sky light before there was any; sky light replaces it and takes its bits.
struct quad {
    uint32 data0 = 0;
    uint32 data1 = 0;

    quad() = default;

    [[nodiscard]] static auto pack(
        vec3i min_pos, vec3i max_pos, uint8 normal_id, block_id block_id, uint8 corners_ao
    ) -> quad;

    // Only the per-instance index is still fed through vertex input; the
    // geometry comes out of a storage buffer.
    [[nodiscard]] static auto get_binding_descriptions()
        -> std::vector<vk::VertexInputBindingDescription>;

    [[nodiscard]] static auto get_attribute_descriptions()
        -> std::vector<vk::VertexInputAttributeDescription>;
};

struct mesh {
    std::vector<quad> quads;

    // The quads are grouped by face direction, in the order +X, -X, +Y, -Y, +Z,
    // -Z, because that is the order the mesher walks. Keeping the six run
    // lengths costs nothing and lets the culling shader drop the three
    // directions that face away from the viewer before they are ever shaded.
    std::array<uint32, 6> face_counts{};

    // Which faces of the chunk see through to which, taken off the same
    // occupancy the mesher runs on. Survives release_data: the culling walk
    // needs it long after the geometry has gone to the device.
    vw::asset::chunk_links links;

    void release_data() {
        quads = {};
    }
};

struct mesh_options {
    // Connectivity is only read by the culling walk, and the walk is off by
    // default. Building it regardless cost a tenth of all meshing.
    bool build_links = false;
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
        std::vector<quad>& quads,
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
    uint8 corner_ao;

    [[nodiscard]]
    auto operator==(const face_mask_cell&) const -> bool = default;

    [[nodiscard]]
    auto is_empty() const -> bool {
        return voxel_id == blocks::air;
    }
};

struct mesh_generation_storage {
    std::vector<quad> quads;
    std::vector<face_mask_cell> mask;
    std::vector<bool> depth_has_pages;

    // 64 KB of bit volume, reused by the worker across chunks and built once
    // per mesh. Heap rather than a member so the worker stack stays small.
    std::unique_ptr<vw::asset::chunk_occupancy> occupancy;
    bool occupancy_valid = false;

    // Reused across chunks so the connectivity fill allocates nothing.
    vw::asset::chunk_link_scratch link_scratch;

    void clear() {
        quads.clear();
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
    std::vector<quad>& quads,
    int face_direction,
    vec3i min_pos,
    vec3i max_pos,
    uint8 palette_index,
    uint8 corner_ao
);

// Takes the mask cell rather than just the block id: the mask already holds the
// ambient occlusion for this quad, and every cell merged into it has the same
// one -- that is what the merge compares on.
// One layer reduced to bit rows: what is visible and what sits in front of the
// face, which is the plane ambient occlusion samples. Empty says the layer
// holds no faces at all, which replaces the linear scan over the whole mask.
struct layer_rows {
    std::array<uint64, 64> visible{};
    std::array<uint64, 64> front{};

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
