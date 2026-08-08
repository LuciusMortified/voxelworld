#pragma once

#ifndef VW_ARENA_APP_H
#define VW_ARENA_APP_H

#include <memory>
#include <vector>

#include <vw/core.h>
#include <vw/gfx.h>

#include "vw/asset/asset_storage.h"
#include "vw/gfx/camera/third_person_camera_controller.h"
#include "vw/gfx/player/player_input_controller.h"
#include "vw/asset/vox/vox_parser_plain.h"
#include "vw/ecs/systems/world_grid/generators/perlin_terrain_generator.h"

#include "dummy_enemy.h"
#include "player.h"

namespace vw::arena {

class arena_app final : public gfx::app {
public:
    explicit arena_app(gfx::engine& eng);
    ~arena_app() override = default;

    void render(float delta_time) override;

private:
    auto handle_key_press(gfx::keyboard::keys key) -> void;
    auto load_assets() -> void;

    asset::vox_parser_plain parser_;
    asset::asset_storage assets_;

    gfx::player_input_controller input_controller_;
    gfx::third_person_camera_controller camera_controller_;

    std::unique_ptr<player> player_;
    std::vector<std::unique_ptr<dummy_enemy>> enemies_;

    gfx::perlin_terrain_generator::params generator_params_;
    bool show_colliders_ = true;
};

}  // namespace vw::arena

#include "arena_app.inl.h"

#endif  // VW_ARENA_APP_H
