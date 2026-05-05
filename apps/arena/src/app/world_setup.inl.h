#pragma once

namespace vw::arena {

inline auto setup_world_grid(gfx::engine<>& engine) -> world_setup_result {
    auto& world    = engine.get_world();
    auto& registry = world.template resource<asset::model_registry>();

    gfx::perlin_terrain_generator::params params{
        .voxel_scale = 16,
    };

    auto generator = std::make_unique<gfx::perlin_terrain_generator>(
        registry.get_identity_pool(), registry.get_page_pool(), params
    );

    auto& gs = world.template system<gfx::world_grid_system>();
    gs.set_grid(std::make_unique<gfx::world_grid<gfx::base_world_def>>(world, params.voxel_scale));
    gs.set_loader(std::make_unique<gfx::chunk_loader>(std::move(generator)));

    return {.generator_params = params};
}

}  // namespace vw::arena
