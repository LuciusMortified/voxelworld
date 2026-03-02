#pragma once

#ifndef VW_GFX_WORLD_GRID_CHUNK_H
#define VW_GFX_WORLD_GRID_CHUNK_H

#include <memory>
#include <vector>

#include "vw/core.h"
#include "vw/core/voxel.h"
#include "vw/gfx/world/entity_guard.h"

namespace vw::gfx {

class model;

using region_id = uint32;

template <typename WC = base_world_components>
class chunk {
public:
    static constexpr int32 size = 128;
    static constexpr int32 volume = size * size * size;

    chunk(world<WC>& world, region_id region, vec3i coord, std::vector<voxel> voxels);
    ~chunk() = default;

    chunk(const chunk&) = delete;
    auto operator=(const chunk&) -> chunk& = delete;
    chunk(chunk&&) noexcept = default;
    auto operator=(chunk&&) noexcept -> chunk& = default;

    [[nodiscard]] auto get_voxel(int32 x, int32 y, int32 z) const -> voxel;
    [[nodiscard]] auto get_voxel(vec3i local) const -> voxel;
    void set_voxel(int32 x, int32 y, int32 z, const voxel& v);
    void set_voxel(vec3i local, const voxel& v);
    [[nodiscard]] auto is_empty(int32 x, int32 y, int32 z) const -> bool;

    [[nodiscard]] auto get_model() const -> std::shared_ptr<model>;
    [[nodiscard]] auto get_entity() const -> entity;
    [[nodiscard]] auto get_region_id() const -> region_id;

    static constexpr auto contains(int32 x, int32 y, int32 z) -> bool {
        return x >= 0 && x < size && y >= 0 && y < size && z >= 0 && z < size;
    }

private:
    entity_guard<WC> guard_;
    std::shared_ptr<model> model_;
    region_id region_id_;
};

}  // namespace vw::gfx

#include "vw/gfx/world_grid/chunk.inl.h"

#endif  // VW_GFX_WORLD_GRID_CHUNK_H
