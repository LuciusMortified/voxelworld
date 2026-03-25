#pragma once

#ifndef VW_GFX_WORLD_GRID_GENERATORS_PERLIN_TERRAIN_GENERATOR_H
#define VW_GFX_WORLD_GRID_GENERATORS_PERLIN_TERRAIN_GENERATOR_H

#include <array>

#include "vw/gfx/world_grid/terrain_generator.h"

namespace vw::gfx {

class model_identity_pool;
class page_pool;

class perlin_terrain_generator final : public terrain_generator {
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

    perlin_terrain_generator(model_identity_pool& identity_pool, page_pool& page_pool,
                             params p = {});

    void generate(terrain_context& ctx) override;
    [[nodiscard]] auto surface_height_at(int32 wx, int32 wz) const -> int32;

private:
    [[nodiscard]] auto noise2d(float64 x, float64 y) const -> float64;
    [[nodiscard]] auto octave_noise(float64 x, float64 y) const -> float64;
    [[nodiscard]] auto ridged_noise(float64 x, float64 y) const -> float64;
    [[nodiscard]] auto continent_at(float64 nx, float64 nz) const -> float64;
    [[nodiscard]] auto height_at(int32 wx, int32 wz) const -> int32;
    [[nodiscard]] auto block_at(int32 y, int32 surface_y, float64 continent) const -> block_id;

    void generate_chunk(terrain_context& ctx, int32 chunk_y);

    static auto fade(float64 t) -> float64;
    static auto lerp(float64 t, float64 a, float64 b) -> float64;
    static auto grad(int32 hash, float64 x, float64 y) -> float64;

    model_identity_pool* identity_pool_;
    page_pool* page_pool_;
    params params_;
    std::array<int32, 512> perm_;
};

}  // namespace vw::gfx

#include "vw/gfx/world_grid/generators/perlin_terrain_generator.inl.h"

#endif  // VW_GFX_WORLD_GRID_GENERATORS_PERLIN_TERRAIN_GENERATOR_H
