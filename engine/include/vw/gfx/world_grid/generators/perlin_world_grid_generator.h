#pragma once

#ifndef VW_GFX_WORLD_GRID_GENERATORS_PERLIN_WORLD_GRID_GENERATOR_H
#define VW_GFX_WORLD_GRID_GENERATORS_PERLIN_WORLD_GRID_GENERATOR_H

#include <array>
#include <unordered_map>

#include "vw/gfx/world_grid/world_grid_generator.h"

namespace vw::gfx {

class perlin_world_grid_generator final : public world_grid_generator {
public:
    struct params {
        uint32 seed = 42;
        int32 voxel_scale = 8;

        int32 shell_depth = 8;

        float32 continent_frequency = 0.003f;
        float32 terrain_frequency = 0.02f;
        int32 octaves = 4;
        float32 lacunarity = 2.0f;
        float32 persistence = 0.5f;

        int32 plains_height = 20;
        int32 hills_height = 35;
        int32 mountains_height = 55;

        float32 ridge_frequency = 0.015f;
        float32 ridge_weight = 0.6f;

        float32 warp_frequency = 0.01f;
        float32 warp_strength = 30.0f;
    };

    explicit perlin_world_grid_generator(params p = {});

    [[nodiscard]] auto generate_chunk(vec3i coord) -> chunk_data override;
    [[nodiscard]] auto get_chunk_y_range(int32 chunk_x, int32 chunk_z) -> chunk_y_range override;
    [[nodiscard]] auto surface_height_at(int32 wx, int32 wz) const -> int32;

private:
    [[nodiscard]] auto noise2d(float64 x, float64 y) const -> float64;
    [[nodiscard]] auto octave_noise(float64 x, float64 y) const -> float64;
    [[nodiscard]] auto ridged_noise(float64 x, float64 y) const -> float64;
    [[nodiscard]] auto continent_at(float64 nx, float64 nz) const -> float64;
    [[nodiscard]] auto height_at(int32 wx, int32 wz) const -> int32;
    [[nodiscard]] auto block_at(int32 y, int32 surface_y, float64 continent) const -> uint8;

    static auto fade(float64 t) -> float64;
    static auto lerp(float64 t, float64 a, float64 b) -> float64;
    static auto grad(int32 hash, float64 x, float64 y) -> float64;

    params params_;
    std::array<int32, 512> perm_;
    std::unordered_map<uint64, chunk_y_range> y_range_cache_;

    static auto pack_coord(int32 cx, int32 cz) -> uint64 {
        return (static_cast<uint64>(static_cast<uint32>(cx)) << 32) |
               static_cast<uint64>(static_cast<uint32>(cz));
    }
};

}  // namespace vw::gfx

#include "vw/gfx/world_grid/generators/perlin_world_grid_generator.inl.h"

#endif  // VW_GFX_WORLD_GRID_GENERATORS_PERLIN_WORLD_GRID_GENERATOR_H
