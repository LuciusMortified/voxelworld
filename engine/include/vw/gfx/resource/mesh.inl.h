#pragma once

#ifndef VW_GFX_MESH_INL_H
#define VW_GFX_MESH_INL_H

namespace vw::gfx {

// ==================== vertex ====================

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
            .format   = VK_FORMAT_R32G32B32_SFLOAT,
            .offset   = offsetof(vertex, position),
        },
        VkVertexInputAttributeDescription{
            .location = 1,
            .binding  = 0,
            .format   = VK_FORMAT_R32G32B32_SFLOAT,
            .offset   = offsetof(vertex, normal),
        },
        VkVertexInputAttributeDescription{
            .location = 2,
            .binding  = 0,
            .format   = VK_FORMAT_R32_UINT,
            .offset   = offsetof(vertex, color),
        },
        VkVertexInputAttributeDescription{
            .location = 3,
            .binding  = 1,
            .format   = VK_FORMAT_R32_UINT,
            .offset   = 0,
        },
        VkVertexInputAttributeDescription{
            .location = 4,
            .binding  = 0,
            .format   = VK_FORMAT_R32_SFLOAT,
            .offset   = offsetof(vertex, ao),
        },
    };
    return attribute_descriptions;
}

// ==================== AO helpers ====================

static constexpr int ao_normal[6][3] = {
    {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}
};

static constexpr int ao_tangent_u[6][3] = {
    {0, 0, 1}, {0, 0, 1}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0}
};

static constexpr int ao_tangent_v[6][3] = {
    {0, 1, 0}, {0, 1, 0}, {0, 0, 1}, {0, 0, 1}, {0, 1, 0}, {0, 1, 0}
};

inline auto compute_vertex_ao(
    const std::shared_ptr<model>& mdl,
    int x, int y, int z,
    int face, int su, int sv
) -> float32 {
    int nx = x + ao_normal[face][0];
    int ny = y + ao_normal[face][1];
    int nz = z + ao_normal[face][2];

    auto is_solid = [&](int px, int py, int pz) -> bool {
        if (px < 0 || px >= mdl->width() ||
            py < 0 || py >= mdl->height() ||
            pz < 0 || pz >= mdl->depth())
            return false;
        return !mdl->is_empty(px, py, pz);
    };

    int ux = ao_tangent_u[face][0], uy = ao_tangent_u[face][1], uz = ao_tangent_u[face][2];
    int vx = ao_tangent_v[face][0], vy = ao_tangent_v[face][1], vz = ao_tangent_v[face][2];

    bool side1 = is_solid(nx + su * ux, ny + su * uy, nz + su * uz);
    bool side2 = is_solid(nx + sv * vx, ny + sv * vy, nz + sv * vz);

    if (side1 && side2) return 0.0f;

    bool corner = is_solid(
        nx + su * ux + sv * vx,
        ny + su * uy + sv * vy,
        nz + su * uz + sv * vz
    );

    int ao_val = 3 - static_cast<int>(side1) - static_cast<int>(side2) - static_cast<int>(corner);
    return static_cast<float32>(ao_val) / 3.0f;
}

// ==================== simple_mesh_generator ====================

// Per-vertex (su, sv) signs for each face direction, matching face_vertices winding
static constexpr int ao_winding_signs[6][4][2] = {
    {{-1, -1}, { 1, -1}, { 1,  1}, {-1,  1}},  // +X
    {{-1, -1}, {-1,  1}, { 1,  1}, { 1, -1}},  // -X
    {{-1, -1}, { 1, -1}, { 1,  1}, {-1,  1}},  // +Y
    {{-1, -1}, {-1,  1}, { 1,  1}, { 1, -1}},  // -Y
    {{-1, -1}, {-1,  1}, { 1,  1}, { 1, -1}},  // +Z
    {{ 1, -1}, { 1,  1}, {-1,  1}, {-1, -1}},  // -Z
};

inline auto simple_mesh_generator::generate_mesh_data(
    const std::shared_ptr<model>& model
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
                            add_cube_face(vertices, indices, model, x, y, z, face, voxel_obj.value);
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
    int x, int y, int z,
    int face_direction,
    color color
) {
    static constexpr vec3f face_normals[6] = {
        vec3f(1, 0, 0),  vec3f(-1, 0, 0),
        vec3f(0, 1, 0),  vec3f(0, -1, 0),
        vec3f(0, 0, 1),  vec3f(0, 0, -1)
    };

    static constexpr vec3f face_vertices[6][4] = {
        {{1, 0, 0}, {1, 0, 1}, {1, 1, 1}, {1, 1, 0}},  // +X
        {{0, 0, 0}, {0, 1, 0}, {0, 1, 1}, {0, 0, 1}},  // -X
        {{0, 1, 0}, {1, 1, 0}, {1, 1, 1}, {0, 1, 1}},  // +Y
        {{0, 0, 0}, {0, 0, 1}, {1, 0, 1}, {1, 0, 0}},  // -Y
        {{0, 0, 1}, {0, 1, 1}, {1, 1, 1}, {1, 0, 1}},  // +Z
        {{1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0, 0, 0}}   // -Z
    };

    const auto base_vertex = static_cast<uint32>(vertices.size());
    vec3f normal = face_normals[face_direction];
    vec3f position(x, y, z);

    std::array<float32, 4> ao_values;
    for (int i = 0; i < 4; i++) {
        ao_values[i] = compute_vertex_ao(
            model, x, y, z, face_direction,
            ao_winding_signs[face_direction][i][0],
            ao_winding_signs[face_direction][i][1]
        );
    }

    for (int i = 0; i < 4; i++) {
        vec3f vertex_pos = position + face_vertices[face_direction][i];
        vertices.emplace_back(vertex_pos, normal, color.value, ao_values[i]);
    }

    if (ao_values[0] + ao_values[2] > ao_values[1] + ao_values[3]) {
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

inline auto simple_mesh_generator::is_face_visible(
    const std::shared_ptr<model>& model, int x, int y, int z, int face_direction
) -> bool {
    if (!model) return false;

    static constexpr int dx[6] = {1, -1, 0, 0, 0, 0};
    static constexpr int dy[6] = {0, 0, 1, -1, 0, 0};
    static constexpr int dz[6] = {0, 0, 0, 0, 1, -1};

    int nx = x + dx[face_direction];
    int ny = y + dy[face_direction];
    int nz = z + dz[face_direction];

    if (nx < 0 || nx >= model->width() || ny < 0 || ny >= model->height() ||
        nz < 0 || nz >= model->depth()) {
        return true;
    }

    return model->is_empty(nx, ny, nz);
}

// ==================== greedy_mesh_generator ====================

struct face_axis_mapping {
    int width, height, depth;
    int face_direction;

    face_axis_mapping(const std::shared_ptr<model>& mdl, int face_dir)
        : face_direction(face_dir) {
        switch (face_dir / 2) {
            case 0:
                width = mdl->depth(); height = mdl->height(); depth = mdl->width();
                break;
            case 1:
                width = mdl->width(); height = mdl->depth(); depth = mdl->height();
                break;
            default:
                width = mdl->width(); height = mdl->height(); depth = mdl->depth();
                break;
        }
    }

    [[nodiscard]] auto to_model_coords(int u, int v, int layer) const
        -> std::tuple<int, int, int> {
        int d = (face_direction % 2 == 0) ? layer : depth - 1 - layer;
        switch (face_direction / 2) {
            case 0: return {d, v, u};
            case 1: return {u, d, v};
            default: return {u, v, d};
        }
    }

    [[nodiscard]] auto to_world_min_max(int u, int v, int w, int h, int layer) const
        -> std::pair<vec3f, vec3f> {
        auto d_lo = static_cast<float32>((face_direction % 2 == 0) ? layer : depth - 1 - layer);
        auto d_hi = d_lo + 1.0f;
        auto u_lo = static_cast<float32>(u);
        auto u_hi = static_cast<float32>(u + w);
        auto v_lo = static_cast<float32>(v);
        auto v_hi = static_cast<float32>(v + h);

        switch (face_direction / 2) {
            case 0: return {{d_lo, v_lo, u_lo}, {d_hi, v_hi, u_hi}};
            case 1: return {{u_lo, d_lo, v_lo}, {u_hi, d_hi, v_hi}};
            default: return {{u_lo, v_lo, d_lo}, {u_hi, v_hi, d_hi}};
        }
    }
};

struct face_cell {
    color col = colors::empty;
    std::array<float32, 4> ao = {1.0f, 1.0f, 1.0f, 1.0f};

    [[nodiscard]] auto is_empty() const -> bool { return col == colors::empty; }
};

// Maps canonical AO order (BL, BR, TR, TL) to face winding vertex order
static constexpr int canonical_to_winding[6][4] = {
    {0, 1, 2, 3},  // +X
    {0, 3, 2, 1},  // -X
    {0, 1, 2, 3},  // +Y
    {0, 3, 2, 1},  // -Y
    {0, 3, 2, 1},  // +Z
    {1, 2, 3, 0},  // -Z
};

inline auto greedy_mesh_generator::generate_mesh_data(
    const std::shared_ptr<model>& model
) -> mesh {
    if (!model) {
        throw std::runtime_error("model is null");
    }

    std::vector<vertex> vertices;
    std::vector<uint32> indices;

    for (int face_direction = 0; face_direction < 6; face_direction++) {
        generate_face_quads(vertices, indices, model, face_direction);
    }

    return mesh{.vertices = std::move(vertices), .indices = std::move(indices)};
}

inline void greedy_mesh_generator::generate_face_quads(
    std::vector<vertex>& vertices,
    std::vector<uint32>& indices,
    const std::shared_ptr<model>& model,
    int face_direction
) {
    face_axis_mapping axes(model, face_direction);

    for (int layer = 0; layer < axes.depth; layer++) {
        std::vector mask(axes.width, std::vector<face_cell>(axes.height));

        for (int u = 0; u < axes.width; u++) {
            for (int v = 0; v < axes.height; v++) {
                auto [mx, my, mz] = axes.to_model_coords(u, v, layer);
                auto voxel = model->get_voxel(mx, my, mz);
                if (!voxel.is_empty() && is_face_visible(model, mx, my, mz, face_direction)) {
                    mask[u][v].col = voxel.value;
                    mask[u][v].ao[0] = compute_vertex_ao(model, mx, my, mz, face_direction, -1, -1);
                    mask[u][v].ao[1] = compute_vertex_ao(model, mx, my, mz, face_direction,  1, -1);
                    mask[u][v].ao[2] = compute_vertex_ao(model, mx, my, mz, face_direction,  1,  1);
                    mask[u][v].ao[3] = compute_vertex_ao(model, mx, my, mz, face_direction, -1,  1);
                }
            }
        }

        std::vector visited(axes.width, std::vector(axes.height, false));
        for (int u = 0; u < axes.width; u++) {
            for (int v = 0; v < axes.height; v++) {
                if (visited[u][v] || mask[u][v].is_empty()) continue;

                auto& origin = mask[u][v];

                int w = 1;
                while (u + w < axes.width && !visited[u + w][v]
                    && mask[u + w][v].col == origin.col
                    && mask[u + w - 1][v].ao[1] == mask[u + w][v].ao[0]
                    && mask[u + w - 1][v].ao[2] == mask[u + w][v].ao[3]) {
                    w++;
                }

                int h = 1;
                bool can_extend = true;
                while (can_extend && v + h < axes.height) {
                    for (int i = 0; i < w; i++) {
                        auto& cell = mask[u + i][v + h];
                        auto& below = mask[u + i][v + h - 1];
                        if (cell.col != origin.col || visited[u + i][v + h]
                            || below.ao[3] != cell.ao[0]
                            || below.ao[2] != cell.ao[1]) {
                            can_extend = false;
                            break;
                        }
                        if (i > 0) {
                            auto& left = mask[u + i - 1][v + h];
                            if (left.ao[1] != cell.ao[0] || left.ao[2] != cell.ao[3]) {
                                can_extend = false;
                                break;
                            }
                        }
                    }
                    if (can_extend) h++;
                }

                for (int i = 0; i < w; i++)
                    for (int j = 0; j < h; j++)
                        visited[u + i][v + j] = true;

                auto [min_pos, max_pos] = axes.to_world_min_max(u, v, w, h, layer);

                std::array<float32, 4> canonical_ao = {
                    mask[u][v].ao[0],
                    mask[u + w - 1][v].ao[1],
                    mask[u + w - 1][v + h - 1].ao[2],
                    mask[u][v + h - 1].ao[3],
                };

                std::array<float32, 4> winding_ao;
                for (int i = 0; i < 4; i++) {
                    winding_ao[i] = canonical_ao[canonical_to_winding[face_direction][i]];
                }

                add_quad(vertices, indices, face_direction, min_pos, max_pos, origin.col, winding_ao);
            }
        }
    }
}

inline void greedy_mesh_generator::add_quad(
    std::vector<vertex>& vertices,
    std::vector<uint32>& indices,
    int face_direction,
    const vec3f& min_pos,
    const vec3f& max_pos,
    color color,
    const std::array<float32, 4>& ao
) {
    static const vec3f face_normals[6] = {
        vec3f(1, 0, 0),  vec3f(-1, 0, 0),
        vec3f(0, 1, 0),  vec3f(0, -1, 0),
        vec3f(0, 0, 1),  vec3f(0, 0, -1)
    };

    // 0 = use min, 1 = use max for each component (x, y, z)
    static constexpr int face_verts[6][4][3] = {
        {{1, 0, 0}, {1, 0, 1}, {1, 1, 1}, {1, 1, 0}},  // +X
        {{0, 0, 0}, {0, 1, 0}, {0, 1, 1}, {0, 0, 1}},  // -X
        {{0, 1, 0}, {1, 1, 0}, {1, 1, 1}, {0, 1, 1}},  // +Y
        {{0, 0, 0}, {0, 0, 1}, {1, 0, 1}, {1, 0, 0}},  // -Y
        {{0, 0, 1}, {0, 1, 1}, {1, 1, 1}, {1, 0, 1}},  // +Z
        {{1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0, 0, 0}},  // -Z
    };

    auto base_vertex = static_cast<uint32>(vertices.size());
    vec3f normal = face_normals[face_direction];

    for (int i = 0; i < 4; i++) {
        vec3f pos{
            face_verts[face_direction][i][0] ? max_pos.x : min_pos.x,
            face_verts[face_direction][i][1] ? max_pos.y : min_pos.y,
            face_verts[face_direction][i][2] ? max_pos.z : min_pos.z,
        };
        vertices.emplace_back(pos, normal, color.value, ao[i]);
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

inline auto greedy_mesh_generator::is_face_visible(
    const std::shared_ptr<model>& model, int x, int y, int z, int face_direction
) -> bool {
    if (!model) return false;

    static constexpr int dx[6] = {1, -1, 0, 0, 0, 0};
    static constexpr int dy[6] = {0, 0, 1, -1, 0, 0};
    static constexpr int dz[6] = {0, 0, 0, 0, 1, -1};

    int nx = x + dx[face_direction];
    int ny = y + dy[face_direction];
    int nz = z + dz[face_direction];

    if (nx < 0 || nx >= model->width() || ny < 0 || ny >= model->height() ||
        nz < 0 || nz >= model->depth()) {
        return true;
    }

    return model->is_empty(nx, ny, nz);
}

}  // namespace vw::gfx

#endif  // VW_GFX_MESH_INL_H
