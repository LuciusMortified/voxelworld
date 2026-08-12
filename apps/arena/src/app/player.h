#pragma once

#ifndef VW_ARENA_PLAYER_H
#define VW_ARENA_PLAYER_H

#include <string_view>

#include <vw/core.h>
#include <vw/gfx.h>

#include "vw/asset/asset_storage.h"
#include "vw/gfx/camera/camera.h"
#include "vw/gfx/player/player_input_state.h"
#include "vw/ecs/systems/world_grid/world_grid.h"

namespace vw::arena {

class player {
public:
    explicit player(gfx::engine& engine, asset::asset_storage& assets);
    ~player();

    player(const player&)                    = delete;
    auto operator=(const player&) -> player& = delete;
    player(player&&)                         = delete;
    auto operator=(player&&) -> player&      = delete;

    auto update(const gfx::player_input_state& input) -> void;
    auto try_place(float32 voxel_scale) -> void;
    auto toggle_sword() -> void;

    [[nodiscard]] auto get_entity() const -> ecs::entity;
    [[nodiscard]] auto has_sword() const -> bool;
    [[nodiscard]] auto is_placed() const -> bool;

private:
    [[nodiscard]] auto create_body_part(
        std::string_view prefab_name, std::string_view part_name
    ) const -> ecs::entity;

    auto handle_attack() const -> void;
    [[nodiscard]] auto can_attack() const -> bool;
    auto setup_animation_fsm() const -> void;

    gfx::engine& engine_;
    asset::asset_storage& assets_;

    ecs::entity root_;
    ecs::entity body_;
    ecs::entity head_;
    ecs::entity hand_right_;
    ecs::entity hand_left_;
    ecs::entity foot_right_;
    ecs::entity foot_left_;
    ecs::entity sword_;

    static constexpr float32 default_rotation_speed_ = 5.0f;
    static constexpr float32 attack_rotation_speed_  = 25.0f;

    bool placed_            = false;
    bool need_update_jump_  = false;
    bool is_attacking_      = false;
    int32 jump_counter_     = 0;
    vec3f attack_facing_{0.0f, 0.0f, 0.0f};
};

}  // namespace vw::arena

#include "player.inl.h"

#endif  // VW_ARENA_PLAYER_H
