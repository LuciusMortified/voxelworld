module vw.world;

import std;

namespace vw::ecs {

world_grid::world_grid(
    world& w, int32 voxel_scale
)
    : world_(&w), voxel_scale_(voxel_scale) {}

auto world_grid::get_voxel(
    vec3i world_pos
) const -> voxel {
    auto cc = world_to_chunk_coord(world_pos);
    auto it = chunks_.find(cc);
    if (it == chunks_.end()) {
        return empty_voxel;
    }
    auto lc = world_to_local_coord(world_pos);
    return it->second->get_voxel(lc / voxel_scale_);
}

void world_grid::set_voxel(
    vec3i world_pos, const voxel& v
) {
    auto cc = world_to_chunk_coord(world_pos);
    auto it = chunks_.find(cc);
    if (it == chunks_.end()) {
        return;
    }
    const auto lc = world_to_local_coord(world_pos) / voxel_scale_;
    it->second->set_voxel(lc, v);

    refresh_chunk(cc);

    // A voxel on the skin of a chunk is half of a seam: the neighbour was
    // meshed against the old plane and has to hear about the new one.
    constexpr int32 last = chunk::size - 1;
    const bool on_face[6]{
        lc.x == last, lc.x == 0, lc.y == last, lc.y == 0, lc.z == last, lc.z == 0,
    };

    for (int32 fd = 0; fd < 6; ++fd) {
        if (on_face[fd]) {
            refresh_chunk(cc + boundary_face_offsets[fd]);
        }
    }
}

void world_grid::refresh_chunk(
    vec3i chunk_coord
) {
    const auto it = chunks_.find(chunk_coord);
    if (it == chunks_.end()) {
        return;
    }

    auto& c    = *it->second;
    auto& mdl  = *c.get_model();
    uint8 mask = 0;

    for (int32 fd = 0; fd < 6; ++fd) {
        const auto neighbor = chunks_.find(chunk_coord + boundary_face_offsets[fd]);
        if (neighbor == chunks_.end()) {
            continue;
        }
        mdl.set_boundary_slice(fd, *neighbor->second->get_model());
        mask |= static_cast<uint8>(1U << fd);
    }

    c.set_known_neighbors(mask);

    // Digging into buried rock is what makes it worth drawing.
    if (c.ensure_entity()) {
        ++drawn_chunks_;
    }

    if (c.get_entity().is_valid()) {
        world_->registry().request_change<model_component>(c.get_entity());
    }
}

auto world_grid::has_chunk(
    vec3i chunk_coord
) const -> bool {
    return chunks_.contains(chunk_coord);
}

auto world_grid::get_chunk(
    vec3i chunk_coord
) -> chunk* {
    auto it = chunks_.find(chunk_coord);
    return it != chunks_.end() ? it->second.get() : nullptr;
}

auto world_grid::get_surface_y(
    int32 wx, int32 wz
) const -> std::optional<int32> {
    constexpr int32 s = chunk::size;

    auto floor_div = [](int32 a, int32 b) -> int32 { return a >= 0 ? a / b : (a - b + 1) / b; };
    int32 cx       = floor_div(wx, s);
    int32 cz       = floor_div(wz, s);
    vec2i col_coord{cx, cz};

    auto col_it = column_chunks_.find(col_coord);
    if (col_it == column_chunks_.end() || col_it->second.empty()) {
        return std::nullopt;
    }

    const auto& y_levels = col_it->second;

    int32 local_x = ((wx % s) + s) % s;
    int32 local_z = ((wz % s) + s) % s;

    for (auto it = y_levels.rbegin(); it != y_levels.rend(); ++it) {
        int32 cy = *it;
        vec3i chunk_coord{cx, cy, cz};
        auto chunk_it = chunks_.find(chunk_coord);
        if (chunk_it == chunks_.end()) {
            continue;
        }

        for (int32 local_y = s - 1; local_y >= 0; --local_y) {
            if (!chunk_it->second->get_voxel(local_x, local_y, local_z).is_empty()) {
                return (cy * s) + local_y;
            }
        }
    }

    return std::nullopt;
}

auto world_grid::has_column(
    vec2i coord
) const -> bool {
    return column_chunks_.contains(coord);
}

auto world_grid::column_levels(
    vec2i coord
) const -> std::span<const int32> {
    const auto it = column_chunks_.find(coord);
    return it != column_chunks_.end() ? std::span<const int32>{it->second}
                                      : std::span<const int32>{};
}

auto world_grid::column_count() const -> uint32 {
    return static_cast<uint32>(column_chunks_.size());
}

auto world_grid::chunk_count() const -> uint32 {
    return static_cast<uint32>(chunks_.size());
}

auto world_grid::drawn_chunk_count() const -> uint32 {
    return drawn_chunks_;
}

auto world_grid::place_chunk(
    vec3i chunk_coord, std::shared_ptr<asset::model> mdl
) -> chunk* {
    auto [it, inserted] = chunks_.emplace(
        chunk_coord,
        std::make_unique<chunk>(*world_, chunk_coord, std::move(mdl), voxel_scale_)
    );

    if (inserted && it->second->is_drawn()) {
        ++drawn_chunks_;
    }

    return it->second.get();
}

void world_grid::register_column(
    vec2i coord, std::vector<int32> y_levels
) {
    column_chunks_[coord] = std::move(y_levels);
}

void world_grid::unload_column(
    vec2i coord
) {
    auto col_it = column_chunks_.find(coord);
    if (col_it != column_chunks_.end()) {
        for (int32 y : col_it->second) {
            vec3i chunk_coord{coord.x, y, coord.y};
            const auto it = chunks_.find(chunk_coord);
            if (it == chunks_.end()) {
                continue;
            }
            if (it->second->is_drawn()) {
                --drawn_chunks_;
            }
            chunks_.erase(it);
        }
        column_chunks_.erase(col_it);
    }
}

auto world_grid::voxel_scale() const -> int32 {
    return voxel_scale_;
}

auto world_grid::world_to_chunk_coord(
    vec3i world_pos
) const -> vec3i {
    const int32 s = chunk::size * voxel_scale_;
    return {
        world_pos.x >= 0 ? world_pos.x / s : (world_pos.x - s + 1) / s,
        world_pos.y >= 0 ? world_pos.y / s : (world_pos.y - s + 1) / s,
        world_pos.z >= 0 ? world_pos.z / s : (world_pos.z - s + 1) / s
    };
}

auto world_grid::world_to_local_coord(
    vec3i world_pos
) const -> vec3i {
    const int32 s = chunk::size * voxel_scale_;
    return {((world_pos.x % s) + s) % s, ((world_pos.y % s) + s) % s, ((world_pos.z % s) + s) % s};
}

auto world_grid::chunk_to_world_coord(
    vec3i chunk_coord
) const -> vec3i {
    const int32 s = chunk::size * voxel_scale_;
    return {chunk_coord.x * s, chunk_coord.y * s, chunk_coord.z * s};
}

}  // namespace vw::ecs
