#pragma once

#ifndef VW_ECS_SYSTEMS_WORLD_GRID_CHUNK_H
#define VW_ECS_SYSTEMS_WORLD_GRID_CHUNK_H

#include <memory>

#include "vw/core.h"
#include "vw/core/voxel.h"
#include "vw/ecs/entity.h"

namespace vw::asset { class model; }

namespace vw::ecs {

class world;

class chunk {
public:
    using world_type = world;

    static constexpr int32 size = 64;
    static constexpr int32 volume = size * size * size;

    chunk(world_type& w, vec3i coord, std::shared_ptr<asset::model> mdl, int32 voxel_scale = 1);
    ~chunk();

    chunk(const chunk&) = delete;
    auto operator=(const chunk&) -> chunk& = delete;
    chunk(chunk&& other) noexcept;
    auto operator=(chunk&& other) noexcept -> chunk&;

    [[nodiscard]] auto get_voxel(int32 x, int32 y, int32 z) const -> voxel;
    [[nodiscard]] auto get_voxel(vec3i local) const -> voxel;
    void set_voxel(int32 x, int32 y, int32 z, const voxel& v) const;
    void set_voxel(vec3i local, const voxel& v) const;
    [[nodiscard]] auto is_empty(int32 x, int32 y, int32 z) const -> bool;

    [[nodiscard]] auto get_model() const -> std::shared_ptr<asset::model>;
    [[nodiscard]] auto get_entity() const -> entity;

    static constexpr auto contains(int32 x, int32 y, int32 z) -> bool {
        return x >= 0 && x < size && y >= 0 && y < size && z >= 0 && z < size;
    }

private:
    world_type* world_;
    entity ent_;
    std::shared_ptr<asset::model> model_;
};

}  // namespace vw::ecs

#endif  // VW_ECS_SYSTEMS_WORLD_GRID_CHUNK_H
