#pragma once

#ifndef VW_GFX_WORLD_GRID_CHUNK_H
#define VW_GFX_WORLD_GRID_CHUNK_H

#include <memory>

#include "vw/core.h"
#include "vw/core/voxel.h"
#include "vw/gfx/world/entity_guard.h"

namespace vw::gfx {

class model;

template <typename WC = base_world_components>
class chunk {
public:
    static constexpr int32 size = 64;
    static constexpr int32 volume = size * size * size;

    chunk(world<WC>& world, vec3i coord, std::shared_ptr<model> model, int32 voxel_scale = 1);
    ~chunk() = default;

    chunk(const chunk&) = delete;
    auto operator=(const chunk&) -> chunk& = delete;
    chunk(chunk&&) noexcept = default;
    auto operator=(chunk&&) noexcept -> chunk& = default;

    [[nodiscard]] auto get_voxel(int32 x, int32 y, int32 z) const -> voxel;
    [[nodiscard]] auto get_voxel(vec3i local) const -> voxel;
    void set_voxel(int32 x, int32 y, int32 z, const voxel& v) const;
    void set_voxel(vec3i local, const voxel& v) const;
    [[nodiscard]] auto is_empty(int32 x, int32 y, int32 z) const -> bool;

    [[nodiscard]] auto get_model() const -> std::shared_ptr<model>;
    [[nodiscard]] auto get_entity() const -> entity;

    static constexpr auto contains(int32 x, int32 y, int32 z) -> bool {
        return x >= 0 && x < size && y >= 0 && y < size && z >= 0 && z < size;
    }

private:
    entity_guard<WC> guard_;
    std::shared_ptr<model> model_;
};

}  // namespace vw::gfx

#include "vw/gfx/world_grid/chunk.inl.h"

#endif  // VW_GFX_WORLD_GRID_CHUNK_H
