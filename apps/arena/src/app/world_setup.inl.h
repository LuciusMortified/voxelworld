#pragma once

namespace vw::arena {

inline auto setup_world_grid(gfx::engine<>& engine) -> world_setup_result {
    auto& world    = engine.get_world();
    auto& registry = world.get_model_registry();

    gfx::perlin_terrain_generator::params params{
        .voxel_scale = 16,
    };

    auto generator = std::make_unique<gfx::perlin_terrain_generator>(
        registry.get_identity_pool(), registry.get_page_pool(), params
    );

    world.set_world_grid(
        std::make_shared<gfx::world_grid<>>(world, std::move(generator), params.voxel_scale)
    );

    return {.generator_params = params};
}

}  // namespace vw::arena
