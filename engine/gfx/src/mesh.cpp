module;

#include <cstddef>

module vw.gfx;

import std;
import vulkan;
import vw.core;
import vw.ecs;
import vw.world;
import vw.platform;
import :vk;

namespace vw::gfx {

// ==================== vertex ====================

auto vertex::pack(
    int x,
    int y,
    int z,
    uint8 normal_id,
    block_id block_id,
    uint8 corners_dark,
    uint8 corners_bright,
    uint8 corner_id
) -> vertex {
    vertex v;
    v.data0 =                                       //
        (static_cast<uint32>(x) & 0x7Fu) |          //
        ((static_cast<uint32>(y) & 0x7Fu) << 7) |   //
        ((static_cast<uint32>(z) & 0x7Fu) << 14) |  //
        ((static_cast<uint32>(normal_id) & 0x7u) << 21);

    v.data1 =                                                   //
        static_cast<uint32>(block_id.value) |                   //
        ((static_cast<uint32>(corners_dark) & 0xFu) << 8) |     //
        ((static_cast<uint32>(corners_bright) & 0xFu) << 12) |  //
        ((static_cast<uint32>(corner_id) & 0x3u) << 16);

    return v;
}

auto vertex::get_binding_descriptions() -> std::vector<vk::VertexInputBindingDescription> {
    std::vector binding_descriptions = {
        vk::VertexInputBindingDescription{
            .binding   = 0,
            .stride    = sizeof(vertex),
            .inputRate = vk::VertexInputRate::eVertex,
        },
        vk::VertexInputBindingDescription{
            .binding   = 1,
            .stride    = sizeof(uint32),
            .inputRate = vk::VertexInputRate::eInstance,
        },
    };
    return binding_descriptions;
}

auto vertex::get_attribute_descriptions() -> std::vector<vk::VertexInputAttributeDescription> {
    std::vector attribute_descriptions = {
        vk::VertexInputAttributeDescription{
            .location = 0,
            .binding  = 0,
            .format   = vk::Format::eR32Uint,
            .offset   = offsetof(vertex, data0),
        },
        vk::VertexInputAttributeDescription{
            .location = 1,
            .binding  = 0,
            .format   = vk::Format::eR32Uint,
            .offset   = offsetof(vertex, data1),
        },
        vk::VertexInputAttributeDescription{
            .location = 2,
            .binding  = 1,
            .format   = vk::Format::eR32Uint,
            .offset   = 0,
        },
    };
    return attribute_descriptions;
}

// ==================== AO tables ====================

static constexpr std::array<vec3i, 6> ao_normal = {
    vec3i{1, 0, 0},
    vec3i{-1, 0, 0},
    vec3i{0, 1, 0},
    vec3i{0, -1, 0},
    vec3i{0, 0, 1},
    vec3i{0, 0, -1},
};

static constexpr std::array<vec3i, 6> ao_tangent_u = {
    vec3i{0, 0, 1},
    vec3i{0, 0, 1},
    vec3i{1, 0, 0},
    vec3i{1, 0, 0},
    vec3i{1, 0, 0},
    vec3i{1, 0, 0},
};

static constexpr std::array<vec3i, 6> ao_tangent_v = {
    vec3i{0, 1, 0},
    vec3i{0, 1, 0},
    vec3i{0, 0, 1},
    vec3i{0, 0, 1},
    vec3i{0, 1, 0},
    vec3i{0, 1, 0},
};

// ==================== detail ====================

namespace detail {

face_axis_mapping::face_axis_mapping(
    const vw::asset::model& mdl, int face_dir
)
    : face_direction(face_dir), voxel_scale(mdl.voxel_scale()) {
    switch (face_dir / 2) {
        case 0:
            width  = mdl.depth();
            height = mdl.height();
            depth  = mdl.width();
            break;
        case 1:
            width  = mdl.width();
            height = mdl.depth();
            depth  = mdl.height();
            break;
        default:
            width  = mdl.width();
            height = mdl.height();
            depth  = mdl.depth();
            break;
    }
}

[[nodiscard]] auto face_axis_mapping::to_model_coords(
    int u, int v, int layer
) const -> std::tuple<int, int, int> {
    int d = (face_direction % 2 == 0) ? layer : depth - 1 - layer;
    switch (face_direction / 2) {
        case 0:
            return {d, v, u};
        case 1:
            return {u, d, v};
        default:
            return {u, v, d};
    }
}

[[nodiscard]] auto face_axis_mapping::to_local_min_max(
    int u, int v, int w, int h, int layer
) const -> std::pair<vec3i, vec3i> {
    int d_lo = (face_direction % 2 == 0) ? layer : depth - 1 - layer;
    int d_hi = d_lo + 1;
    int u_hi = u + w;
    int v_hi = v + h;

    switch (face_direction / 2) {
        case 0:
            return {{d_lo, v, u}, {d_hi, v_hi, u_hi}};
        case 1:
            return {{u, d_lo, v}, {u_hi, d_hi, v_hi}};
        default:
            return {{u, v, d_lo}, {u_hi, v_hi, d_hi}};
    }
}

auto is_solid_at(
    const vw::asset::model& mdl, vec3i p
) -> bool {
    const bool ox = p.x < 0 || p.x >= mdl.width();
    const bool oy = p.y < 0 || p.y >= mdl.height();
    const bool oz = p.z < 0 || p.z >= mdl.depth();

    if (!ox && !oy && !oz) {
        return !mdl.is_empty(p.x, p.y, p.z);
    }
    if (static_cast<int>(ox) + static_cast<int>(oy) + static_cast<int>(oz) > 1) {
        return false;
    }

    if (p.x >= mdl.width() && mdl.has_boundary_slice(0)) {
        return mdl.is_boundary_solid(0, 0, p.y, p.z);
    }
    if (p.x < 0 && mdl.has_boundary_slice(1)) {
        return mdl.is_boundary_solid(1, 0, p.y, p.z);
    }
    if (p.y >= mdl.height() && mdl.has_boundary_slice(2)) {
        return mdl.is_boundary_solid(2, p.x, 0, p.z);
    }
    if (p.y < 0 && mdl.has_boundary_slice(3)) {
        return mdl.is_boundary_solid(3, p.x, 0, p.z);
    }
    if (p.z >= mdl.depth() && mdl.has_boundary_slice(4)) {
        return mdl.is_boundary_solid(4, p.x, p.y, 0);
    }
    if (p.z < 0 && mdl.has_boundary_slice(5)) {
        return mdl.is_boundary_solid(5, p.x, p.y, 0);
    }

    return false;
}

auto compute_corner_darkness(
    const vw::asset::model& mdl, int x, int y, int z, int face
) -> uint8 {
    const vec3i n = vec3i{x, y, z} + ao_normal[face];
    const vec3i u = ao_tangent_u[face];
    const vec3i v = ao_tangent_v[face];

    const bool edge_mv = is_solid_at(mdl, n - v);
    const bool edge_pu = is_solid_at(mdl, n + u);
    const bool edge_pv = is_solid_at(mdl, n + v);
    const bool edge_mu = is_solid_at(mdl, n - u);

    const bool diag_c0 = is_solid_at(mdl, n - u - v);
    const bool diag_c1 = is_solid_at(mdl, n + u - v);
    const bool diag_c2 = is_solid_at(mdl, n + u + v);
    const bool diag_c3 = is_solid_at(mdl, n - u + v);

    auto corner = [](bool s1, bool s2, bool d) -> uint8 {
        if (s1 && s2) {
            return 3;
        }
        return static_cast<uint8>(s1) + static_cast<uint8>(s2) + static_cast<uint8>(d);
    };

    const uint8 c0 = corner(edge_mu, edge_mv, diag_c0);
    const uint8 c1 = corner(edge_pu, edge_mv, diag_c1);
    const uint8 c2 = corner(edge_pu, edge_pv, diag_c2);
    const uint8 c3 = corner(edge_mu, edge_pv, diag_c3);

    return static_cast<uint8>(c0 | (c1 << 2) | (c2 << 4) | (c3 << 6));
}

auto compute_corner_brightness(
    const vw::asset::model& mdl, int x, int y, int z, int face
) -> uint8 {
    const vec3i host = vec3i{x, y, z};
    const vec3i u    = ao_tangent_u[face];
    const vec3i v    = ao_tangent_v[face];

    const bool miss_mv = !is_solid_at(mdl, host - v);
    const bool miss_pu = !is_solid_at(mdl, host + u);
    const bool miss_pv = !is_solid_at(mdl, host + v);
    const bool miss_mu = !is_solid_at(mdl, host - u);

    const bool miss_c0 = !is_solid_at(mdl, host - u - v);
    const bool miss_c1 = !is_solid_at(mdl, host + u - v);
    const bool miss_c2 = !is_solid_at(mdl, host + u + v);
    const bool miss_c3 = !is_solid_at(mdl, host - u + v);

    auto corner = [](bool s1, bool s2, bool d) -> uint8 {
        if (s1 && s2) {
            return 3;
        }
        return static_cast<uint8>(s1) + static_cast<uint8>(s2) + static_cast<uint8>(d);
    };

    const uint8 c0 = corner(miss_mu, miss_mv, miss_c0);
    const uint8 c1 = corner(miss_pu, miss_mv, miss_c1);
    const uint8 c2 = corner(miss_pu, miss_pv, miss_c2);
    const uint8 c3 = corner(miss_mu, miss_pv, miss_c3);

    return static_cast<uint8>(c0 | (c1 << 2) | (c2 << 4) | (c3 << 6));
}

auto is_face_visible(
    const vw::asset::model& mdl, int x, int y, int z, int face_direction
) -> bool {
    static constexpr int dx[6] = {1, -1, 0, 0, 0, 0};
    static constexpr int dy[6] = {0, 0, 1, -1, 0, 0};
    static constexpr int dz[6] = {0, 0, 0, 0, 1, -1};

    int nx = x + dx[face_direction];
    int ny = y + dy[face_direction];
    int nz = z + dz[face_direction];

    if (nx < 0 || nx >= mdl.width() || ny < 0 || ny >= mdl.height() || nz < 0 ||
        nz >= mdl.depth()) {
        if (mdl.has_boundary_slice(face_direction)) {
            return !mdl.is_boundary_solid(face_direction, x, y, z);
        }
        return true;
    }

    return mdl.is_empty(nx, ny, nz);
}

void build_face_mask(
    mesh_generation_storage& storage,
    const vw::asset::model& mdl,
    const face_axis_mapping& axes,
    int face_direction,
    int layer,
    mesh_options opts
) {
    const bool brightness_enabled = opts.enable_top_brightness && face_direction == 2;
    constexpr int ps = vw::asset::model::page_size;

    auto idx = [&](int u, int v) -> size_t {
        return (static_cast<size_t>(u) * static_cast<size_t>(axes.height)) + static_cast<size_t>(v);
    };

    constexpr face_mask_cell empty_cell{blocks::air, 0, 0};

    for (int u_block = 0; u_block < axes.width; u_block += ps) {
        int u_end = std::min(u_block + ps, axes.width);
        for (int v_block = 0; v_block < axes.height; v_block += ps) {
            int v_end = std::min(v_block + ps, axes.height);

            auto [pmx, pmy, pmz] = axes.to_model_coords(u_block, v_block, layer);
            auto pm              = mdl.get_page_mode(pmx / ps, pmy / ps, pmz / ps);

            if (pm == vw::asset::page_mode::empty) {
                for (int u = u_block; u < u_end; u++) {
                    for (int v = v_block; v < v_end; v++) {
                        storage.mask[idx(u, v)] = empty_cell;
                    }
                }
                continue;
            }

            if (pm == vw::asset::page_mode::uniform) {
                const auto fid = mdl.get_page_fill_id(pmx / ps, pmy / ps, pmz / ps);
                for (int u = u_block; u < u_end; u++) {
                    for (int v = v_block; v < v_end; v++) {
                        auto [mx, my, mz] = axes.to_model_coords(u, v, layer);
                        if (is_face_visible(mdl, mx, my, mz, face_direction)) {
                            storage.mask[idx(u, v)] = {
                                fid,
                                compute_corner_darkness(mdl, mx, my, mz, face_direction),
                                brightness_enabled
                                    ? compute_corner_brightness(mdl, mx, my, mz, face_direction)
                                    : uint8{0}
                            };
                        } else {
                            storage.mask[idx(u, v)] = empty_cell;
                        }
                    }
                }
                continue;
            }

            auto* page = mdl.get_page(pmx / ps, pmy / ps, pmz / ps);
            for (int u = u_block; u < u_end; u++) {
                for (int v = v_block; v < v_end; v++) {
                    auto [mx, my, mz] = axes.to_model_coords(u, v, layer);
                    const int lx      = mx % ps;
                    const int ly      = my % ps;
                    const int lz      = mz % ps;
                    auto& vx          = (*page)[lx + ly * ps + lz * ps * ps];
                    if (!vx.is_empty() && is_face_visible(mdl, mx, my, mz, face_direction)) {
                        storage.mask[idx(u, v)] = {
                            vx.id,
                            compute_corner_darkness(mdl, mx, my, mz, face_direction),
                            brightness_enabled
                                ? compute_corner_brightness(mdl, mx, my, mz, face_direction)
                                : uint8{0}
                        };
                    } else {
                        storage.mask[idx(u, v)] = empty_cell;
                    }
                }
            }
        }
    }
}

void add_quad(
    std::vector<vertex>& vertices,
    std::vector<uint32>& indices,
    int face_direction,
    vec3i min_pos,
    vec3i max_pos,
    block_id block_id,
    uint8 corner_dark,
    uint8 corner_bright
) {
    static constexpr int face_verts[6][4][3] = {
        {{1, 0, 0}, {1, 0, 1}, {1, 1, 1}, {1, 1, 0}},  // +X
        {{0, 0, 0}, {0, 1, 0}, {0, 1, 1}, {0, 0, 1}},  // -X
        {{0, 1, 0}, {1, 1, 0}, {1, 1, 1}, {0, 1, 1}},  // +Y
        {{0, 0, 0}, {0, 0, 1}, {1, 0, 1}, {1, 0, 0}},  // -Y
        {{0, 0, 1}, {0, 1, 1}, {1, 1, 1}, {1, 0, 1}},  // +Z
        {{1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0, 0, 0}},  // -Z
    };

    static constexpr uint8 winding_to_corner[6][4] = {
        {0, 1, 3, 2},  // +X
        {0, 2, 3, 1},  // -X
        {0, 1, 3, 2},  // +Y
        {0, 2, 3, 1},  // -Y
        {0, 2, 3, 1},  // +Z
        {1, 3, 2, 0},  // -Z
    };

    static constexpr uint8 corner_to_ao[4] = {0, 1, 3, 2};

    const auto normal_id   = static_cast<uint8>(face_direction);
    const auto base_vertex = static_cast<uint32>(vertices.size());

    const uint8 c_dark[4] = {
        static_cast<uint8>(corner_dark & 0x3u),
        static_cast<uint8>((corner_dark >> 2) & 0x3u),
        static_cast<uint8>((corner_dark >> 4) & 0x3u),
        static_cast<uint8>((corner_dark >> 6) & 0x3u),
    };
    const uint8 c_bright[4] = {
        static_cast<uint8>(corner_bright & 0x3u),
        static_cast<uint8>((corner_bright >> 2) & 0x3u),
        static_cast<uint8>((corner_bright >> 4) & 0x3u),
        static_cast<uint8>((corner_bright >> 6) & 0x3u),
    };

    uint8 corners_dark_winding   = 0;
    uint8 corners_bright_winding = 0;
    for (int i = 0; i < 4; i++) {
        const uint8 corner_i = winding_to_corner[face_direction][i];
        const uint8 ao_i     = corner_to_ao[corner_i];
        if (c_dark[ao_i] > 0) {
            corners_dark_winding |= static_cast<uint8>(1u << i);
        }
        if (c_bright[ao_i] > 0) {
            corners_bright_winding |= static_cast<uint8>(1u << i);
        }
    }

    for (int i = 0; i < 4; i++) {
        const int x = face_verts[face_direction][i][0] ? max_pos.x : min_pos.x;
        const int y = face_verts[face_direction][i][1] ? max_pos.y : min_pos.y;
        const int z = face_verts[face_direction][i][2] ? max_pos.z : min_pos.z;
        vertices.push_back(vertex::pack(
            x, y, z, normal_id, block_id,
            corners_dark_winding, corners_bright_winding, static_cast<uint8>(i)
        ));
    }

    indices.push_back(base_vertex + 0);
    indices.push_back(base_vertex + 1);
    indices.push_back(base_vertex + 2);
    indices.push_back(base_vertex + 2);
    indices.push_back(base_vertex + 3);
    indices.push_back(base_vertex + 0);
}


// The neighbour plane in the orientation this face needs. For +-Y and +-Z the
// stored plane is already rows of u; +-X keeps y-major rows, so that one is
// gathered bit by bit -- it happens on one layer out of sixty-four.
auto boundary_row(
    const vw::asset::model& mdl, int face_direction, int v
) -> uint64 {
    if (!mdl.has_boundary_slice(face_direction)) {
        return 0;  // no neighbour: the far side is open, every face is visible
    }

    const auto& face = mdl.get_boundary_face(face_direction);

    if (face_direction / 2 == 0) {
        uint64 bits = 0;
        for (int z = 0; z < 64; ++z) {
            if (face.test(v, z)) {
                bits |= uint64{1} << z;
            }
        }
        return bits;
    }

    return face.rows[v];
}

auto build_layer_rows(
    const vw::asset::model& mdl,
    const vw::asset::chunk_occupancy& occupancy,
    const face_axis_mapping& axes,
    int face_direction,
    int layer,
    layer_rows& out
) -> bool {
    const int d    = (face_direction % 2 == 0) ? layer : axes.depth - 1 - layer;
    const int step = (face_direction % 2 == 0) ? 1 : -1;
    const int nd   = d + step;
    const bool inside = nd >= 0 && nd < 64;

    out.front_outside = !inside;

    uint64 any = 0;

    for (int v = 0; v < 64; ++v) {
        uint64 own  = 0;
        uint64 front = 0;

        switch (face_direction / 2) {
            case 0:  // +-X: rows of z at (y = v, x = layer plane)
                own   = occupancy.zrow(v, d);
                front = inside ? occupancy.zrow(v, nd) : boundary_row(mdl, face_direction, v);
                break;
            case 1:  // +-Y: rows of x at (y = layer plane, z = v)
                own   = occupancy.row(d, v);
                front = inside ? occupancy.row(nd, v) : boundary_row(mdl, face_direction, v);
                break;
            default:  // +-Z: rows of x at (y = v, z = layer plane)
                own   = occupancy.row(v, d);
                front = inside ? occupancy.row(v, nd) : boundary_row(mdl, face_direction, v);
                break;
        }

        out.own[v]     = own;
        out.front[v]   = front;
        out.visible[v] = own & ~front;
        any |= out.visible[v];
    }

    return any != 0;
}

// Ambient occlusion for one cell, read out of three neighbouring bit rows
// instead of eight lookups into the paged volume. Rows are pre-shifted so the
// eight samples around a cell are all bit u of some mask.
struct corner_samples {
    uint64 edge_mu = 0;
    uint64 edge_pu = 0;
    uint64 edge_mv = 0;
    uint64 edge_pv = 0;
    uint64 diag_c0 = 0;
    uint64 diag_c1 = 0;
    uint64 diag_c2 = 0;
    uint64 diag_c3 = 0;
};

auto samples_from_rows(uint64 row_mv, uint64 row, uint64 row_pv) -> corner_samples {
    return {
        .edge_mu = row << 1,
        .edge_pu = row >> 1,
        .edge_mv = row_mv,
        .edge_pv = row_pv,
        .diag_c0 = row_mv << 1,
        .diag_c1 = row_mv >> 1,
        .diag_c2 = row_pv >> 1,
        .diag_c3 = row_pv << 1,
    };
}

auto pack_corners(const corner_samples& s, int u) -> uint8 {
    const auto bit = [u](uint64 mask) -> bool { return ((mask >> u) & 1U) != 0; };

    auto corner = [](bool s1, bool s2, bool diag) -> uint8 {
        if (s1 && s2) {
            return 3;
        }
        return static_cast<uint8>(s1) + static_cast<uint8>(s2) + static_cast<uint8>(diag);
    };

    const uint8 c0 = corner(bit(s.edge_mu), bit(s.edge_mv), bit(s.diag_c0));
    const uint8 c1 = corner(bit(s.edge_pu), bit(s.edge_mv), bit(s.diag_c1));
    const uint8 c2 = corner(bit(s.edge_pu), bit(s.edge_pv), bit(s.diag_c2));
    const uint8 c3 = corner(bit(s.edge_mu), bit(s.edge_pv), bit(s.diag_c3));

    return static_cast<uint8>(c0 | (c1 << 2) | (c2 << 4) | (c3 << 6));
}

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
) {
    auto [min_pos, max_pos] = axes.to_local_min_max(u_start, v_start, w, h, layer);

    add_quad(
        storage.vertices,
        storage.indices,
        face_direction,
        min_pos,
        max_pos,
        block_id{cell.voxel_id},
        cell.corner_dark,
        cell.corner_bright
    );
}

}  // namespace detail

// ==================== simple_mesh_generator ====================

auto simple_mesh_generator::generate_mesh_data(
    const std::shared_ptr<vw::asset::model>& mdl, const block_registry& registry, mesh_options opts
) -> mesh {
    if (!mdl) {
        return mesh{};
    }

    std::vector<vertex> vertices;
    std::vector<uint32> indices;

    for (int x = 0; x < mdl->width(); x++) {
        for (int y = 0; y < mdl->height(); y++) {
            for (int z = 0; z < mdl->depth(); z++) {
                if (voxel voxel_obj = mdl->get_voxel(x, y, z); !voxel_obj.is_empty()) {
                    for (int face = 0; face < 6; face++) {
                        if (is_face_visible(mdl, x, y, z, face)) {
                            add_cube_face(
                                vertices, indices, mdl, x, y, z, face, voxel_obj.id, registry, opts
                            );
                        }
                    }
                }
            }
        }
    }

    return mesh{.vertices = std::move(vertices), .indices = std::move(indices)};
}

void simple_mesh_generator::add_cube_face(
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
) {
    uint8 corner_dark   = detail::compute_corner_darkness(*mdl, x, y, z, face_direction);
    uint8 corner_bright = (opts.enable_top_brightness && face_direction == 2)
        ? detail::compute_corner_brightness(*mdl, x, y, z, face_direction)
        : uint8{0};
    detail::add_quad(
        vertices,
        indices,
        face_direction,
        {x, y, z},
        {x + 1, y + 1, z + 1},
        voxel_id,
        corner_dark,
        corner_bright
    );
}

auto simple_mesh_generator::is_face_visible(
    const std::shared_ptr<vw::asset::model>& mdl, int x, int y, int z, int face_direction
) -> bool {
    if (!mdl)
        return false;

    static constexpr int dx[6] = {1, -1, 0, 0, 0, 0};
    static constexpr int dy[6] = {0, 0, 1, -1, 0, 0};
    static constexpr int dz[6] = {0, 0, 0, 0, 1, -1};

    int nx = x + dx[face_direction];
    int ny = y + dy[face_direction];
    int nz = z + dz[face_direction];

    if (nx < 0 || nx >= mdl->width() || ny < 0 || ny >= mdl->height() || nz < 0 ||
        nz >= mdl->depth()) {
        return true;
    }

    return mdl->is_empty(nx, ny, nz);
}

// ==================== strip_mesh_generator ====================

auto strip_mesh_generator::generate_mesh_data(
    mesh_generation_storage& storage,
    const vw::asset::model& mdl,
    const block_registry& registry,
    mesh_options opts
) -> mesh {
    storage.clear();

    auto total    = mdl.width() * mdl.height() * mdl.depth();
    auto estimate = static_cast<size_t>(total / 4);

    if (storage.vertices.capacity() < estimate) {
        storage.vertices.reserve(estimate);
    }
    if (storage.indices.capacity() < estimate) {
        storage.indices.reserve(estimate);
    }

    for (int face_direction = 0; face_direction < 6; face_direction++) {
        generate_face_quads(storage, mdl, face_direction, registry, opts);
    }

    // Moved, not copied: the storage exists to be reused, and copying it back
    // out was two allocations and a memcpy of the whole mesh per chunk. The
    // reserve at the top of the next call restores the capacity.
    return mesh{std::move(storage.vertices), std::move(storage.indices)};
}

void strip_mesh_generator::merge_and_emit_strips(
    mesh_generation_storage& storage,
    const vw::asset::model& mdl,
    const detail::face_axis_mapping& axes,
    int face_direction,
    int layer,
    const block_registry& registry,
    mesh_options opts
) {
    auto idx = [&](int u, int v) -> size_t {
        return static_cast<size_t>(u) * static_cast<size_t>(axes.height) + static_cast<size_t>(v);
    };

    for (int v = 0; v < axes.height; v++) {
        int u = 0;
        while (u < axes.width) {
            face_mask_cell cell = storage.mask[idx(u, v)];
            if (cell.is_empty()) {
                u++;
                continue;
            }

            int strip_start = u;
            u++;
            while (u < axes.width && storage.mask[idx(u, v)] == cell) {
                u++;
            }
            int w = u - strip_start;

            detail::emit_rect(storage, axes, face_direction, layer, strip_start, v, w, 1, cell);
        }
    }
}

void strip_mesh_generator::generate_face_quads(
    mesh_generation_storage& storage,
    const vw::asset::model& mdl,
    int face_direction,
    const block_registry& registry,
    mesh_options opts
) {
    detail::face_axis_mapping axes(mdl, face_direction);
    constexpr int ps = vw::asset::model::page_size;

    auto mask_size = static_cast<size_t>(axes.width) * static_cast<size_t>(axes.height);
    storage.mask.resize(mask_size);

    const int depth_pages = (axes.depth + ps - 1) / ps;
    const int u_pages     = (axes.width + ps - 1) / ps;
    const int v_pages     = (axes.height + ps - 1) / ps;

    storage.depth_has_pages.resize(depth_pages);
    std::ranges::fill(storage.depth_has_pages, false);
    for (int pd = 0; pd < depth_pages; pd++) {
        for (int pu = 0; pu < u_pages && !storage.depth_has_pages[pd]; pu++) {
            for (int pv = 0; pv < v_pages && !storage.depth_has_pages[pd]; pv++) {
                auto [mx, my, mz] = axes.to_model_coords(pu * ps, pv * ps, pd * ps);
                if (mdl.get_page_mode(mx / ps, my / ps, mz / ps) != vw::asset::page_mode::empty) {
                    storage.depth_has_pages[pd] = true;
                }
            }
        }
    }

    for (int layer = 0; layer < axes.depth; layer++) {
        if (!storage.depth_has_pages[layer / ps]) {
            layer = ((layer / ps) + 1) * ps - 1;
            continue;
        }

        detail::build_face_mask(storage, mdl, axes, face_direction, layer, opts);

        bool has_faces = false;
        for (size_t i = 0; i < mask_size && !has_faces; i++) {
            has_faces = !storage.mask[i].is_empty();
        }
        if (!has_faces)
            continue;

        merge_and_emit_strips(storage, mdl, axes, face_direction, layer, registry, opts);
    }
}

// ==================== greedy_mesh_generator ====================

auto greedy_mesh_generator::generate_mesh_data(
    mesh_generation_storage& storage,
    const vw::asset::model& mdl,
    const block_registry& registry,
    mesh_options opts
) -> mesh {
    storage.clear();

    auto total    = mdl.width() * mdl.height() * mdl.depth();
    auto estimate = static_cast<size_t>(total / 4);

    if (storage.vertices.capacity() < estimate) {
        storage.vertices.reserve(estimate);
    }
    if (storage.indices.capacity() < estimate) {
        storage.indices.reserve(estimate);
    }

    // Built once for the whole mesh and read by all six directions. Models that
    // are not 64-cubes (Sculptor's) fall back to the per-cell path.
    if (!storage.occupancy) {
        storage.occupancy = std::make_unique<vw::asset::chunk_occupancy>();
    }
    storage.occupancy_valid = mdl.build_occupancy(*storage.occupancy);

    for (int face_direction = 0; face_direction < 6; face_direction++) {
        generate_face_quads(storage, mdl, face_direction, registry, opts);
    }

    // Moved, not copied: the storage exists to be reused, and copying it back
    // out was two allocations and a memcpy of the whole mesh per chunk. The
    // reserve at the top of the next call restores the capacity.
    return mesh{std::move(storage.vertices), std::move(storage.indices)};
}

void greedy_mesh_generator::merge_and_emit_rects(
    mesh_generation_storage& storage,
    const vw::asset::model& mdl,
    const detail::face_axis_mapping& axes,
    int face_direction,
    int layer,
    const block_registry& registry,
    mesh_options opts
) {
    auto idx = [&](int u, int v) -> size_t {
        return static_cast<size_t>(u) * static_cast<size_t>(axes.height) + static_cast<size_t>(v);
    };

    face_mask_cell empty_cell{blocks::air, 0, 0};

    for (int v = 0; v < axes.height; v++) {
        for (int u = 0; u < axes.width; u++) {
            face_mask_cell cell = storage.mask[idx(u, v)];
            if (cell.is_empty())
                continue;

            int w = 1;
            while (u + w < axes.width && storage.mask[idx(u + w, v)] == cell) {
                w++;
            }

            int h = 1;
            while (v + h < axes.height) {
                bool row_ok = true;
                for (int du = 0; du < w; du++) {
                    if (storage.mask[idx(u + du, v + h)] != cell) {
                        row_ok = false;
                        break;
                    }
                }
                if (!row_ok)
                    break;
                h++;
            }

            for (int dv = 0; dv < h; dv++) {
                for (int du = 0; du < w; du++) {
                    storage.mask[idx(u + du, v + dv)] = empty_cell;
                }
            }

            detail::emit_rect(storage, axes, face_direction, layer, u, v, w, h, cell);
        }
    }
}

void greedy_mesh_generator::generate_face_quads(
    mesh_generation_storage& storage,
    const vw::asset::model& mdl,
    int face_direction,
    const block_registry& registry,
    mesh_options opts
) {
    detail::face_axis_mapping axes(mdl, face_direction);
    constexpr int ps = vw::asset::model::page_size;

    auto mask_size = static_cast<size_t>(axes.width) * static_cast<size_t>(axes.height);
    storage.mask.resize(mask_size);

    int depth_pages = (axes.depth + ps - 1) / ps;
    int u_pages     = (axes.width + ps - 1) / ps;
    int v_pages     = (axes.height + ps - 1) / ps;

    storage.depth_has_pages.resize(depth_pages);
    std::ranges::fill(storage.depth_has_pages, false);
    for (int pd = 0; pd < depth_pages; pd++) {
        for (int pu = 0; pu < u_pages && !storage.depth_has_pages[pd]; pu++) {
            for (int pv = 0; pv < v_pages && !storage.depth_has_pages[pd]; pv++) {
                auto [mx, my, mz] = axes.to_model_coords(pu * ps, pv * ps, pd * ps);
                if (mdl.get_page_mode(mx / ps, my / ps, mz / ps) != vw::asset::page_mode::empty) {
                    storage.depth_has_pages[pd] = true;
                }
            }
        }
    }

    auto idx = [&](int u, int v) -> size_t {
        return static_cast<size_t>(u) * static_cast<size_t>(axes.height) + static_cast<size_t>(v);
    };

    detail::layer_rows rows;

    for (int layer = 0; layer < axes.depth; layer++) {
        if (!storage.depth_has_pages[layer / ps]) {
            layer = ((layer / ps) + 1) * ps - 1;
            continue;
        }

        if (storage.occupancy_valid) {
            // Visibility for a whole row of 64 cells is one and-not, and an
            // empty layer is known without touching the mask at all. Only the
            // cells that actually carry a face are written.
            if (!detail::build_layer_rows(
                    mdl, *storage.occupancy, axes, face_direction, layer, rows
                )) {
                continue;
            }

            const bool want_bright = opts.enable_top_brightness && face_direction == 2;

            for (int v = 0; v < axes.height; v++) {
                uint64 bits = rows.visible[v];
                if (bits == 0) {
                    continue;
                }

                // Occlusion samples sit one cell away in u and v, so a cell on
                // the edge of the layer reaches into the neighbouring chunk and
                // keeps the scalar path, which knows how to ask for it. Same
                // when the sampled plane is outside the chunk entirely.
                const bool interior_v = v > 0 && v + 1 < axes.height;
                const bool bit_ao     = interior_v && !rows.front_outside;

                const auto dark = detail::samples_from_rows(
                    interior_v ? rows.front[v - 1] : 0,
                    rows.front[v],
                    interior_v ? rows.front[v + 1] : 0
                );
                const auto bright = detail::samples_from_rows(
                    interior_v ? ~rows.own[v - 1] : 0,
                    ~rows.own[v],
                    interior_v ? ~rows.own[v + 1] : 0
                );

                while (bits != 0) {
                    const int u = std::countr_zero(bits);
                    bits &= bits - 1;

                    auto [mx, my, mz] = axes.to_model_coords(u, v, layer);

                    const bool interior = bit_ao && u > 0 && u + 1 < axes.width;

                    face_mask_cell cell{};
                    cell.voxel_id = mdl.get_voxel(mx, my, mz).id;

                    if (interior) {
                        cell.corner_dark   = detail::pack_corners(dark, u);
                        cell.corner_bright = want_bright ? detail::pack_corners(bright, u)
                                                         : uint8{0};
                    } else {
                        cell.corner_dark = detail::compute_corner_darkness(
                            mdl, mx, my, mz, face_direction
                        );
                        cell.corner_bright =
                            want_bright
                                ? detail::compute_corner_brightness(mdl, mx, my, mz, face_direction)
                                : uint8{0};
                    }

                    storage.mask[idx(u, v)] = cell;
                }
            }

            merge_and_emit_rects(storage, mdl, axes, face_direction, layer, registry, opts);
            continue;
        }

        detail::build_face_mask(storage, mdl, axes, face_direction, layer, opts);

        bool has_faces = false;
        for (size_t i = 0; i < mask_size && !has_faces; i++) {
            has_faces = !storage.mask[i].is_empty();
        }
        if (!has_faces)
            continue;

        merge_and_emit_rects(storage, mdl, axes, face_direction, layer, registry, opts);
    }
}

}  // namespace vw::gfx
