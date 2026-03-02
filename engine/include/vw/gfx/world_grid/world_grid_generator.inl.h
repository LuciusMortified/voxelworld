#pragma once

#ifndef VW_GFX_WORLD_GRID_GENERATOR_INL_H
#define VW_GFX_WORLD_GRID_GENERATOR_INL_H

#include "vw/core/color.h"
#include "vw/gfx/model/model.h"

namespace vw::gfx {

inline flat_world_grid_generator::flat_world_grid_generator(
    int32 height, int32 region_size, int32 voxel_scale
)
    : height_(height), region_size_(region_size), voxel_scale_(voxel_scale) {}

inline auto flat_world_grid_generator::get_region_id(
    int32 cx, int32 cz
) const -> region_id {
    int32 rx = cx >= 0 ? cx / region_size_ : (cx - region_size_ + 1) / region_size_;
    int32 rz = cz >= 0 ? cz / region_size_ : (cz - region_size_ + 1) / region_size_;
    auto ux = static_cast<uint32>(rx + 32768);
    auto uz = static_cast<uint32>(rz + 32768);
    return (ux << 16) | uz;
}

inline auto flat_world_grid_generator::generate_region(
    region_id id
) -> region_gen_result {
    const auto ux = static_cast<int32>((id >> 16) & 0xFFFF) - 32768;
    const auto uz = static_cast<int32>(id & 0xFFFF) - 32768;

    int32 cx_min = ux * region_size_;
    int32 cz_min = uz * region_size_;
    int32 cx_max = cx_min + region_size_ - 1;
    int32 cz_max = cz_min + region_size_ - 1;

    region_gen_result result;
    result.meta.id = id;
    result.meta.aabb_min = {cx_min, cz_min};
    result.meta.aabb_max = {cx_max, cz_max};

    constexpr int32 s = chunk<>::size;

    for (int32 cx = cx_min; cx <= cx_max; ++cx) {
        for (int32 cz = cz_min; cz <= cz_max; ++cz) {
            chunk_data data;
            data.region = id;
            data.coord = {cx, 0, cz};

            auto mdl = std::make_shared<model>(*pool_, s, s, s, voxel_scale_);

            bool checker = ((cx + cz) & 1) == 0;
            auto grass = checker ? color{60, 140, 50, 255} : color{80, 170, 60, 255};

            for (int32 x = 0; x < s; ++x) {
                for (int32 z = 0; z < s; ++z) {
                    for (int32 y = 0; y < height_; ++y) {
                        if (y == height_ - 1) {
                            mdl->set_voxel(x, y, z, grass);
                        } else {
                            mdl->set_voxel(x, y, z, colors::brown);
                        }
                    }
                }
            }

            data.model = std::move(mdl);
            result.chunks.push_back(std::move(data));
        }
    }

    return result;
}

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_GRID_GENERATOR_INL_H
