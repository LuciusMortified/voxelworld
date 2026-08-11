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
    auto lc = world_to_local_coord(world_pos);
    it->second->set_voxel(lc / voxel_scale_, v);
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

auto world_grid::column_count() const -> uint32 {
    return static_cast<uint32>(column_chunks_.size());
}

auto world_grid::chunk_count() const -> uint32 {
    return static_cast<uint32>(chunks_.size());
}

auto world_grid::place_chunk(
    vec3i chunk_coord, std::shared_ptr<asset::model> mdl
) -> chunk* {
    auto [it, inserted] = chunks_.emplace(
        chunk_coord,
        std::make_unique<chunk>(*world_, chunk_coord, std::move(mdl), voxel_scale_)
    );
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
            chunks_.erase(chunk_coord);
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
