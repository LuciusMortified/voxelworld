#pragma once

#ifndef VW_ECS_SYSTEMS_WORLD_GRID_H
#define VW_ECS_SYSTEMS_WORLD_GRID_H

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "vw/core.h"
#include "vw/ecs/systems/world_grid/chunk.h"

namespace vw::asset { class model; }

namespace vw::ecs {

template <typename>
class world;

template <typename WD>
class world_grid {
    using world_type = world<WD>;
    using chunk_type = chunk<WD>;

public:
    explicit world_grid(world_type& w, int32 voxel_scale = 8);
    ~world_grid() = default;

    world_grid(const world_grid&)                    = delete;
    auto operator=(const world_grid&) -> world_grid& = delete;
    world_grid(world_grid&&)                         = delete;
    auto operator=(world_grid&&) -> world_grid&      = delete;

    [[nodiscard]] auto get_voxel(vec3i world_pos) const -> voxel;
    void set_voxel(vec3i world_pos, const voxel& v);

    [[nodiscard]] auto has_chunk(vec3i chunk_coord) const -> bool;
    [[nodiscard]] auto get_chunk(vec3i chunk_coord) -> chunk_type*;

    [[nodiscard]] auto get_surface_y(int32 wx, int32 wz) const -> std::optional<int32>;
    [[nodiscard]] auto has_column(vec2i coord) const -> bool;
    [[nodiscard]] auto column_count() const -> uint32;
    [[nodiscard]] auto chunk_count() const -> uint32;

    auto place_chunk(vec3i chunk_coord, std::shared_ptr<asset::model> mdl) -> chunk_type*;
    void register_column(vec2i coord, std::vector<int32> y_levels);
    void unload_column(vec2i coord);

    [[nodiscard]] auto voxel_scale() const -> int32;

    [[nodiscard]] auto world_to_chunk_coord(vec3i world_pos) const -> vec3i;
    [[nodiscard]] auto world_to_local_coord(vec3i world_pos) const -> vec3i;
    [[nodiscard]] auto chunk_to_world_coord(vec3i chunk_coord) const -> vec3i;

private:
    world_type* world_;
    int32 voxel_scale_{1};
    std::unordered_map<vec3i, std::unique_ptr<chunk_type>> chunks_;
    std::unordered_map<vec2i, std::vector<int32>> column_chunks_;
};

}  // namespace vw::ecs

#include "vw/ecs/systems/world_grid/world_grid.inl.h"

#endif  // VW_ECS_SYSTEMS_WORLD_GRID_H
