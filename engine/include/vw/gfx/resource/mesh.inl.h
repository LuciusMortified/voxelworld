#pragma once

#ifndef VW_GFX_MESH_INL_H
#define VW_GFX_MESH_INL_H

namespace vw::gfx {

// ==================== vertex ====================

inline auto vertex::pack(
    int x, int y, int z, uint8 normal_id, uint8 ao, uint8 palette_index
) -> vertex {
    vertex v;
    v.data0 = (static_cast<uint32>(x) & 0x7Fu)
            | ((static_cast<uint32>(y) & 0x7Fu) << 7)
            | ((static_cast<uint32>(z) & 0x7Fu) << 14)
            | ((static_cast<uint32>(normal_id) & 0x7u) << 21)
            | ((static_cast<uint32>(ao) & 0x3u) << 24);
    v.data1 = static_cast<uint32>(palette_index);
    return v;
}

inline auto vertex::get_binding_descriptions() -> std::vector<VkVertexInputBindingDescription> {
    std::vector binding_descriptions = {
        VkVertexInputBindingDescription{
            .binding   = 0,
            .stride    = sizeof(vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        },
        VkVertexInputBindingDescription{
            .binding   = 1,
            .stride    = sizeof(uint32),
            .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE,
        },
    };
    return binding_descriptions;
}

inline auto vertex::get_attribute_descriptions() -> std::vector<VkVertexInputAttributeDescription> {
    std::vector attribute_descriptions = {
        VkVertexInputAttributeDescription{
            .location = 0,
            .binding  = 0,
            .format   = VK_FORMAT_R32_UINT,
            .offset   = offsetof(vertex, data0),
        },
        VkVertexInputAttributeDescription{
            .location = 1,
            .binding  = 0,
            .format   = VK_FORMAT_R32_UINT,
            .offset   = offsetof(vertex, data1),
        },
        VkVertexInputAttributeDescription{
            .location = 2,
            .binding  = 1,
            .format   = VK_FORMAT_R32_UINT,
            .offset   = 0,
        },
    };
    return attribute_descriptions;
}

// ==================== AO tables ====================

static constexpr int ao_normal[6][3] = {
    {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}
};

static constexpr int ao_tangent_u[6][3] = {
    {0, 0, 1}, {0, 0, 1}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0}
};

static constexpr int ao_tangent_v[6][3] = {
    {0, 1, 0}, {0, 1, 0}, {0, 0, 1}, {0, 0, 1}, {0, 1, 0}, {0, 1, 0}
};

static constexpr int ao_winding_signs[6][4][2] = {
    {{-1, -1}, {1, -1}, {1, 1}, {-1, 1}},  // +X
    {{-1, -1}, {-1, 1}, {1, 1}, {1, -1}},  // -X
    {{-1, -1}, {1, -1}, {1, 1}, {-1, 1}},  // +Y
    {{-1, -1}, {-1, 1}, {1, 1}, {1, -1}},  // -Y
    {{-1, -1}, {-1, 1}, {1, 1}, {1, -1}},  // +Z
    {{1, -1}, {1, 1}, {-1, 1}, {-1, -1}},  // -Z
};

static constexpr int canonical_to_winding[6][4] = {
    {0, 1, 2, 3},  // +X
    {0, 3, 2, 1},  // -X
    {0, 1, 2, 3},  // +Y
    {0, 3, 2, 1},  // -Y
    {0, 3, 2, 1},  // +Z
    {1, 2, 3, 0},  // -Z
};

// ==================== detail ====================

namespace detail {

inline face_axis_mapping::face_axis_mapping(const model& mdl, int face_dir)
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

[[nodiscard]] inline auto face_axis_mapping::to_model_coords(
    int u, int v, int layer
) const -> std::tuple<int, int, int> {
    int d = (face_direction % 2 == 0) ? layer : depth - 1 - layer;
    switch (face_direction / 2) {
        case 0:  return {d, v, u};
        case 1:  return {u, d, v};
        default: return {u, v, d};
    }
}

[[nodiscard]] inline auto face_axis_mapping::to_local_min_max(
    int u, int v, int w, int h, int layer
) const -> std::pair<vec3i, vec3i> {
    int d_lo = (face_direction % 2 == 0) ? layer : depth - 1 - layer;
    int d_hi = d_lo + 1;
    int u_hi = u + w;
    int v_hi = v + h;

    switch (face_direction / 2) {
        case 0:  return {{d_lo, v, u}, {d_hi, v_hi, u_hi}};
        case 1:  return {{u, d_lo, v}, {u_hi, d_hi, v_hi}};
        default: return {{u, v, d_lo}, {u_hi, v_hi, d_hi}};
    }
}

inline auto compute_vertex_ao_int(
    const model& mdl, int x, int y, int z, int face, int su, int sv
) -> int {
    int nx = x + ao_normal[face][0];
    int ny = y + ao_normal[face][1];
    int nz = z + ao_normal[face][2];

    auto is_solid = [&](int px, int py, int pz) -> bool {
        bool ox = px < 0 || px >= mdl.width();
        bool oy = py < 0 || py >= mdl.height();
        bool oz = pz < 0 || pz >= mdl.depth();

        if (!ox && !oy && !oz) return !mdl.is_empty(px, py, pz);
        if ((ox ? 1 : 0) + (oy ? 1 : 0) + (oz ? 1 : 0) > 1) return false;

        if (px >= mdl.width() && mdl.has_boundary_slice(0))
            return mdl.is_boundary_solid(0, 0, py, pz);
        if (px < 0 && mdl.has_boundary_slice(1))
            return mdl.is_boundary_solid(1, 0, py, pz);
        if (py >= mdl.height() && mdl.has_boundary_slice(2))
            return mdl.is_boundary_solid(2, px, 0, pz);
        if (py < 0 && mdl.has_boundary_slice(3))
            return mdl.is_boundary_solid(3, px, 0, pz);
        if (pz >= mdl.depth() && mdl.has_boundary_slice(4))
            return mdl.is_boundary_solid(4, px, py, 0);
        if (pz < 0 && mdl.has_boundary_slice(5))
            return mdl.is_boundary_solid(5, px, py, 0);

        return false;
    };

    int ux = ao_tangent_u[face][0], uy = ao_tangent_u[face][1], uz = ao_tangent_u[face][2];
    int vx = ao_tangent_v[face][0], vy = ao_tangent_v[face][1], vz = ao_tangent_v[face][2];

    bool side1 = is_solid(nx + su * ux, ny + su * uy, nz + su * uz);
    bool side2 = is_solid(nx + sv * vx, ny + sv * vy, nz + sv * vz);

    if (side1 && side2) return 0;

    bool corner = is_solid(
        nx + su * ux + sv * vx, ny + su * uy + sv * vy, nz + su * uz + sv * vz
    );

    return 3 - static_cast<int>(side1) - static_cast<int>(side2) - static_cast<int>(corner);
}

inline auto compute_vertex_ao_float(
    const model& mdl, int x, int y, int z, int face, int su, int sv
) -> float32 {
    return static_cast<float32>(compute_vertex_ao_int(mdl, x, y, z, face, su, sv)) / 3.0f;
}

inline auto compute_ao_key(
    const model& mdl, int x, int y, int z, int face
) -> uint8 {
    int a0 = compute_vertex_ao_int(mdl, x, y, z, face, -1, -1);
    int a1 = compute_vertex_ao_int(mdl, x, y, z, face,  1, -1);
    int a2 = compute_vertex_ao_int(mdl, x, y, z, face,  1,  1);
    int a3 = compute_vertex_ao_int(mdl, x, y, z, face, -1,  1);
    return static_cast<uint8>(a0 | (a1 << 2) | (a2 << 4) | (a3 << 6));
}

inline auto is_face_visible(
    const model& mdl, int x, int y, int z, int face_direction
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

inline void build_face_mask(
    mesh_generation_storage& storage,
    const model& mdl,
    const face_axis_mapping& axes,
    int face_direction,
    int layer
) {
    constexpr int ps = model::page_size;

    auto idx = [&](int u, int v) -> size_t {
        return static_cast<size_t>(u) * static_cast<size_t>(axes.height) + static_cast<size_t>(v);
    };

    face_mask_cell empty_cell{0, 0};

    for (int u_block = 0; u_block < axes.width; u_block += ps) {
        int u_end = std::min(u_block + ps, axes.width);
        for (int v_block = 0; v_block < axes.height; v_block += ps) {
            int v_end = std::min(v_block + ps, axes.height);

            auto [pmx, pmy, pmz] = axes.to_model_coords(u_block, v_block, layer);
            auto pm = mdl.get_page_mode(pmx / ps, pmy / ps, pmz / ps);

            if (pm == page_mode::empty) {
                for (int u = u_block; u < u_end; u++)
                    for (int v = v_block; v < v_end; v++)
                        storage.mask[idx(u, v)] = empty_cell;
                continue;
            }

            if (pm == page_mode::uniform) {
                uint8 fid = mdl.get_page_fill_id(pmx / ps, pmy / ps, pmz / ps);
                for (int u = u_block; u < u_end; u++) {
                    for (int v = v_block; v < v_end; v++) {
                        auto [mx, my, mz] = axes.to_model_coords(u, v, layer);
                        if (is_face_visible(mdl, mx, my, mz, face_direction)) {
                            storage.mask[idx(u, v)] = {
                                fid, compute_ao_key(mdl, mx, my, mz, face_direction)
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
                    int lx = mx % ps, ly = my % ps, lz = mz % ps;
                    auto& vx = (*page)[lx + ly * ps + lz * ps * ps];
                    if (!vx.is_empty() && is_face_visible(mdl, mx, my, mz, face_direction)) {
                        storage.mask[idx(u, v)] = {
                            vx.id, compute_ao_key(mdl, mx, my, mz, face_direction)
                        };
                    } else {
                        storage.mask[idx(u, v)] = empty_cell;
                    }
                }
            }
        }
    }
}

inline void add_quad(
    std::vector<vertex>& vertices,
    std::vector<uint32>& indices,
    int face_direction,
    vec3i min_pos,
    vec3i max_pos,
    uint8 palette_index,
    const std::array<uint8, 4>& ao
) {
    static constexpr int face_verts[6][4][3] = {
        {{1, 0, 0}, {1, 0, 1}, {1, 1, 1}, {1, 1, 0}},  // +X
        {{0, 0, 0}, {0, 1, 0}, {0, 1, 1}, {0, 0, 1}},  // -X
        {{0, 1, 0}, {1, 1, 0}, {1, 1, 1}, {0, 1, 1}},  // +Y
        {{0, 0, 0}, {0, 0, 1}, {1, 0, 1}, {1, 0, 0}},  // -Y
        {{0, 0, 1}, {0, 1, 1}, {1, 1, 1}, {1, 0, 1}},  // +Z
        {{1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0, 0, 0}},  // -Z
    };

    auto normal_id = static_cast<uint8>(face_direction);
    auto base_vertex = static_cast<uint32>(vertices.size());

    for (int i = 0; i < 4; i++) {
        int x = face_verts[face_direction][i][0] ? max_pos.x : min_pos.x;
        int y = face_verts[face_direction][i][1] ? max_pos.y : min_pos.y;
        int z = face_verts[face_direction][i][2] ? max_pos.z : min_pos.z;
        vertices.push_back(vertex::pack(x, y, z, normal_id, ao[i], palette_index));
    }

    if (ao[0] + ao[2] > ao[1] + ao[3]) {
        indices.push_back(base_vertex + 0);
        indices.push_back(base_vertex + 1);
        indices.push_back(base_vertex + 2);
        indices.push_back(base_vertex + 2);
        indices.push_back(base_vertex + 3);
        indices.push_back(base_vertex + 0);
    } else {
        indices.push_back(base_vertex + 1);
        indices.push_back(base_vertex + 2);
        indices.push_back(base_vertex + 3);
        indices.push_back(base_vertex + 3);
        indices.push_back(base_vertex + 0);
        indices.push_back(base_vertex + 1);
    }
}

inline void emit_rect(
    mesh_generation_storage& storage,
    const model& mdl,
    const face_axis_mapping& axes,
    int face_direction,
    int layer,
    int u_start, int v_start, int w, int h,
    uint8 voxel_id,
    const block_registry& registry
) {
    auto [min_pos, max_pos] = axes.to_local_min_max(u_start, v_start, w, h, layer);

    auto [c0x, c0y, c0z] = axes.to_model_coords(u_start, v_start, layer);
    auto [c1x, c1y, c1z] = axes.to_model_coords(u_start + w - 1, v_start, layer);
    auto [c2x, c2y, c2z] = axes.to_model_coords(u_start + w - 1, v_start + h - 1, layer);
    auto [c3x, c3y, c3z] = axes.to_model_coords(u_start, v_start + h - 1, layer);

    std::array canonical_ao = {
        static_cast<uint8>(compute_vertex_ao_int(mdl, c0x, c0y, c0z, face_direction, -1, -1)),
        static_cast<uint8>(compute_vertex_ao_int(mdl, c1x, c1y, c1z, face_direction,  1, -1)),
        static_cast<uint8>(compute_vertex_ao_int(mdl, c2x, c2y, c2z, face_direction,  1,  1)),
        static_cast<uint8>(compute_vertex_ao_int(mdl, c3x, c3y, c3z, face_direction, -1,  1)),
    };

    std::array<uint8, 4> winding_ao;
    for (int k = 0; k < 4; k++) {
        winding_ao[k] = canonical_ao[canonical_to_winding[face_direction][k]];
    }

    add_quad(storage.vertices, storage.indices, face_direction, min_pos, max_pos, voxel_id, winding_ao);
}

}  // namespace detail

// ==================== simple_mesh_generator ====================

inline auto simple_mesh_generator::generate_mesh_data(
    const std::shared_ptr<model>& model, const block_registry& registry
) -> mesh {
    if (!model) {
        return mesh{};
    }

    std::vector<vertex> vertices;
    std::vector<uint32> indices;

    for (int x = 0; x < model->width(); x++) {
        for (int y = 0; y < model->height(); y++) {
            for (int z = 0; z < model->depth(); z++) {
                if (voxel voxel_obj = model->get_voxel(x, y, z); !voxel_obj.is_empty()) {
                    for (int face = 0; face < 6; face++) {
                        if (is_face_visible(model, x, y, z, face)) {
                            add_cube_face(vertices, indices, model, x, y, z, face,
                                          voxel_obj.id, registry);
                        }
                    }
                }
            }
        }
    }

    return mesh{.vertices = std::move(vertices), .indices = std::move(indices)};
}

inline void simple_mesh_generator::add_cube_face(
    std::vector<vertex>& vertices,
    std::vector<uint32>& indices,
    const std::shared_ptr<model>& model,
    int x,
    int y,
    int z,
    int face_direction,
    uint8 voxel_id,
    const block_registry& registry
) {
    auto normal_id = static_cast<uint8>(face_direction);

    std::array<uint8, 4> ao_values;
    for (int i = 0; i < 4; i++) {
        ao_values[i] = static_cast<uint8>(detail::compute_vertex_ao_int(
            *model, x, y, z, face_direction,
            ao_winding_signs[face_direction][i][0],
            ao_winding_signs[face_direction][i][1]
        ));
    }

    detail::add_quad(vertices, indices, face_direction,
                     {x, y, z}, {x + 1, y + 1, z + 1}, voxel_id, ao_values);
}

inline auto simple_mesh_generator::is_face_visible(
    const std::shared_ptr<model>& model, int x, int y, int z, int face_direction
) -> bool {
    if (!model)
        return false;

    static constexpr int dx[6] = {1, -1, 0, 0, 0, 0};
    static constexpr int dy[6] = {0, 0, 1, -1, 0, 0};
    static constexpr int dz[6] = {0, 0, 0, 0, 1, -1};

    int nx = x + dx[face_direction];
    int ny = y + dy[face_direction];
    int nz = z + dz[face_direction];

    if (nx < 0 || nx >= model->width() || ny < 0 || ny >= model->height() || nz < 0 ||
        nz >= model->depth()) {
        return true;
    }

    return model->is_empty(nx, ny, nz);
}

// ==================== strip_mesh_generator ====================

inline auto strip_mesh_generator::generate_mesh_data(
    mesh_generation_storage& storage, const model& mdl, const block_registry& registry
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
        generate_face_quads(storage, mdl, face_direction, registry);
    }

    return mesh{storage.vertices, storage.indices};
}

inline void strip_mesh_generator::merge_and_emit_strips(
    mesh_generation_storage& storage,
    const model& mdl,
    const detail::face_axis_mapping& axes,
    int face_direction,
    int layer,
    const block_registry& registry
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

            detail::emit_rect(storage, mdl, axes, face_direction, layer,
                              strip_start, v, w, 1, cell.voxel_id, registry);
        }
    }
}

inline void strip_mesh_generator::generate_face_quads(
    mesh_generation_storage& storage, const model& mdl, int face_direction,
    const block_registry& registry
) {
    detail::face_axis_mapping axes(mdl, face_direction);
    constexpr int ps = model::page_size;

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
                if (mdl.get_page_mode(mx / ps, my / ps, mz / ps) != page_mode::empty) {
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

        detail::build_face_mask(storage, mdl, axes, face_direction, layer);

        bool has_faces = false;
        for (size_t i = 0; i < mask_size && !has_faces; i++) {
            has_faces = !storage.mask[i].is_empty();
        }
        if (!has_faces) continue;

        merge_and_emit_strips(storage, mdl, axes, face_direction, layer, registry);
    }
}

// ==================== greedy_mesh_generator ====================

inline auto greedy_mesh_generator::generate_mesh_data(
    mesh_generation_storage& storage, const model& mdl, const block_registry& registry
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
        generate_face_quads(storage, mdl, face_direction, registry);
    }

    return mesh{storage.vertices, storage.indices};
}

inline void greedy_mesh_generator::merge_and_emit_rects(
    mesh_generation_storage& storage,
    const model& mdl,
    const detail::face_axis_mapping& axes,
    int face_direction,
    int layer,
    const block_registry& registry
) {
    auto idx = [&](int u, int v) -> size_t {
        return static_cast<size_t>(u) * static_cast<size_t>(axes.height) + static_cast<size_t>(v);
    };

    face_mask_cell empty_cell{0, 0};

    for (int v = 0; v < axes.height; v++) {
        for (int u = 0; u < axes.width; u++) {
            face_mask_cell cell = storage.mask[idx(u, v)];
            if (cell.is_empty()) continue;

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
                if (!row_ok) break;
                h++;
            }

            for (int dv = 0; dv < h; dv++) {
                for (int du = 0; du < w; du++) {
                    storage.mask[idx(u + du, v + dv)] = empty_cell;
                }
            }

            detail::emit_rect(storage, mdl, axes, face_direction, layer,
                              u, v, w, h, cell.voxel_id, registry);
        }
    }
}

inline void greedy_mesh_generator::generate_face_quads(
    mesh_generation_storage& storage, const model& mdl, int face_direction,
    const block_registry& registry
) {
    detail::face_axis_mapping axes(mdl, face_direction);
    constexpr int ps = model::page_size;

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
                if (mdl.get_page_mode(mx / ps, my / ps, mz / ps) != page_mode::empty) {
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

        detail::build_face_mask(storage, mdl, axes, face_direction, layer);

        bool has_faces = false;
        for (size_t i = 0; i < mask_size && !has_faces; i++) {
            has_faces = !storage.mask[i].is_empty();
        }
        if (!has_faces) continue;

        merge_and_emit_rects(storage, mdl, axes, face_direction, layer, registry);
    }
}

}  // namespace vw::gfx

#endif  // VW_GFX_MESH_INL_H
