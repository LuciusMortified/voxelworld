#pragma once

#ifndef VW_ECS_WORLD_GRID_GENERATORS_PERLIN_WORLD_GRID_GENERATOR_INL_H
#define VW_ECS_WORLD_GRID_GENERATORS_PERLIN_WORLD_GRID_GENERATOR_INL_H

#include <algorithm>
#include <cmath>
#include <numeric>

#include "vw/core/color.h"
#include "vw/asset/model/model.h"

namespace vw::ecs {


inline perlin_world_grid_generator::perlin_world_grid_generator(
    vw::asset::model_identity_pool& identity_pool, vw::asset::page_pool& pool, params p
)
    : identity_pool_(&identity_pool), page_pool_(&pool), params_(p) {
    for (int32 i = 0; i < 256; ++i) {
        perm_[i] = i;
    }

    uint32 state = params_.seed;
    for (int32 i = 255; i > 0; --i) {
        state   = state * 1664525u + 1013904223u;
        int32 j = static_cast<int32>(state % static_cast<uint32>(i + 1));
        std::swap(perm_[i], perm_[j]);
    }

    for (int32 i = 0; i < 256; ++i) {
        perm_[i + 256] = perm_[i];
    }
}

inline auto perlin_world_grid_generator::fade(
    float64 t
) -> float64 {
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

inline auto perlin_world_grid_generator::lerp(
    float64 t, float64 a, float64 b
) -> float64 {
    return a + t * (b - a);
}

inline auto perlin_world_grid_generator::grad(
    int32 hash, float64 x, float64 y
) -> float64 {
    int32 h   = hash & 3;
    float64 u = h < 2 ? x : y;
    float64 v = h < 2 ? y : x;
    return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

inline auto perlin_world_grid_generator::noise2d(
    float64 x, float64 y
) const -> float64 {
    int32 xi = static_cast<int32>(std::floor(x)) & 255;
    int32 yi = static_cast<int32>(std::floor(y)) & 255;

    float64 xf = x - std::floor(x);
    float64 yf = y - std::floor(y);

    float64 u = fade(xf);
    float64 v = fade(yf);

    int32 aa = perm_[perm_[xi] + yi];
    int32 ab = perm_[perm_[xi] + yi + 1];
    int32 ba = perm_[perm_[xi + 1] + yi];
    int32 bb = perm_[perm_[xi + 1] + yi + 1];

    float64 x1 = lerp(u, grad(aa, xf, yf), grad(ba, xf - 1.0, yf));
    float64 x2 = lerp(u, grad(ab, xf, yf - 1.0), grad(bb, xf - 1.0, yf - 1.0));

    return lerp(v, x1, x2);
}

inline auto perlin_world_grid_generator::octave_noise(
    float64 x, float64 y
) const -> float64 {
    float64 total   = 0.0;
    float64 freq    = 1.0;
    float64 amp     = 1.0;
    float64 max_amp = 0.0;

    for (int32 i = 0; i < params_.octaves; ++i) {
        total += noise2d(x * freq, y * freq) * amp;
        max_amp += amp;
        freq *= params_.lacunarity;
        amp *= params_.persistence;
    }

    return total / max_amp;
}

inline auto perlin_world_grid_generator::ridged_noise(
    float64 x, float64 y
) const -> float64 {
    float64 total   = 0.0;
    float64 freq    = 1.0;
    float64 amp     = 1.0;
    float64 max_amp = 0.0;

    for (int32 i = 0; i < params_.octaves; ++i) {
        float64 n = noise2d(x * freq, y * freq);
        n         = 1.0 - std::abs(n);
        n         = n * n;
        total += n * amp;
        max_amp += amp;
        freq *= params_.lacunarity;
        amp *= params_.persistence;
    }

    return total / max_amp;
}

inline auto perlin_world_grid_generator::continent_at(
    float64 nx, float64 nz
) const -> float64 {
    float64 c = octave_noise(nx * params_.continent_frequency, nz * params_.continent_frequency);
    return (c + 1.0) * 0.5;
}

inline auto perlin_world_grid_generator::height_at(
    int32 wx, int32 wz
) const -> int32 {
    auto nx = static_cast<float64>(wx);
    auto nz = static_cast<float64>(wz);

    float64 warp_x =
        octave_noise(nx * params_.warp_frequency + 5.2, nz * params_.warp_frequency + 1.3) *
        params_.warp_strength;
    float64 warp_z =
        octave_noise(nx * params_.warp_frequency + 9.7, nz * params_.warp_frequency + 6.1) *
        params_.warp_strength;

    float64 warped_x = nx + warp_x;
    float64 warped_z = nz + warp_z;

    float64 continent = continent_at(nx, nz);

    float64 terrain =
        octave_noise(warped_x * params_.terrain_frequency, warped_z * params_.terrain_frequency);
    terrain = (terrain + 1.0) * 0.5;

    float64 ridge =
        ridged_noise(warped_x * params_.ridge_frequency, warped_z * params_.ridge_frequency);

    float64 mixed = terrain * (1.0 - params_.ridge_weight) + ridge * params_.ridge_weight;

    int32 base_h;
    float64 amplitude;
    if (continent < 0.35) {
        base_h    = params_.plains_height;
        amplitude = 5.0;
    } else if (continent < 0.55) {
        float64 t = (continent - 0.35) / 0.2;
        base_h    = params_.plains_height +
            static_cast<int32>(t * (params_.hills_height - params_.plains_height));
        amplitude = 5.0 + t * 15.0;
    } else if (continent < 0.75) {
        float64 t = (continent - 0.55) / 0.2;
        base_h    = params_.hills_height +
            static_cast<int32>(t * (params_.mountains_height - params_.hills_height));
        amplitude = 20.0 + t * 20.0;
    } else {
        base_h    = params_.mountains_height;
        amplitude = 40.0;
    }

    return base_h + static_cast<int32>(mixed * amplitude);
}

inline auto perlin_world_grid_generator::block_at(
    int32 y, int32 surface_y, float64 continent
) const -> block_id {
    int32 depth = surface_y - y;

    if (continent >= 0.7) {
        if (depth == 0) {
            if (surface_y > params_.mountains_height + 20)
                return blocks::gray_9;
            return blocks::gray_5;
        }
        return blocks::gray_4;
    }

    if (depth == 0) {
        if (surface_y > params_.mountains_height + 10)
            return blocks::gray_9;
        if (surface_y > params_.hills_height + 10)
            return blocks::gray_4;
        return blocks::green_2;
    }
    if (depth < 3)
        return blocks::brown_0;
    return blocks::gray_3;
}

inline auto perlin_world_grid_generator::get_chunk_y_range(
    int32 chunk_x, int32 chunk_z
) -> chunk_y_range {
    auto key = pack_coord(chunk_x, chunk_z);
    if (auto it = y_range_cache_.find(key); it != y_range_cache_.end()) {
        return it->second;
    }

    constexpr int32 s      = chunk<>::size;
    constexpr int32 margin = 10;

    int32 wx0 = chunk_x * s;
    int32 wz0 = chunk_z * s;
    int32 wx1 = wx0 + s - 1;
    int32 wz1 = wz0 + s - 1;
    int32 wxm = wx0 + s / 2;
    int32 wzm = wz0 + s / 2;

    int32 samples[] = {
        height_at(wx0, wz0),
        height_at(wx1, wz0),
        height_at(wx0, wz1),
        height_at(wx1, wz1),
        height_at(wxm, wzm),
    };

    int32 min_h = *std::min_element(std::begin(samples), std::end(samples)) - margin;
    int32 max_h = *std::max_element(std::begin(samples), std::end(samples)) + margin;

    int32 bottom = min_h - params_.shell_depth;

    auto floor_div = [](int32 a, int32 b) -> int32 { return a >= 0 ? a / b : (a - b + 1) / b; };

    chunk_y_range result{floor_div(bottom, s), floor_div(max_h, s)};
    y_range_cache_[key] = result;
    return result;
}

inline auto perlin_world_grid_generator::surface_height_at(
    int32 wx, int32 wz
) const -> int32 {
    return height_at(wx, wz);
}

inline auto perlin_world_grid_generator::generate_chunk(
    vec3i coord
) -> chunk_data {
    constexpr int32 s = chunk<>::size;

    auto mdl = std::make_shared<vw::asset::model>(*identity_pool_, *page_pool_, s, s, s, params_.voxel_scale);

    for (int32 x = 0; x < s; ++x) {
        for (int32 z = 0; z < s; ++z) {
            int32 wx      = coord.x * s + x;
            int32 wz      = coord.z * s + z;
            int32 surface = height_at(wx, wz);

            auto nx           = static_cast<float64>(wx);
            auto nz           = static_cast<float64>(wz);
            float64 continent = continent_at(nx, nz);

            int32 top    = std::min(surface, coord.y * s + s - 1);
            int32 bottom = std::max(coord.y * s, surface - params_.shell_depth);

            for (int32 wy = bottom; wy <= top; ++wy) {
                int32 y = wy - coord.y * s;
                if (y < 0 || y >= s)
                    continue;
                mdl->set_voxel(x, y, z, voxel{block_at(wy, surface, continent)});
            }
        }
    }

    return {coord, std::move(mdl)};
}

}  // namespace vw::ecs

#endif  // VW_ECS_WORLD_GRID_GENERATORS_PERLIN_WORLD_GRID_GENERATOR_INL_H
