#pragma once

#ifndef VW_ARENA_PLAYER_H
#define VW_ARENA_PLAYER_H

#include <memory>
#include <string_view>

#include <vw/core.h>
#include <vw/gfx.h>

#include "vw/gfx/asset_storage.h"
#include "vw/gfx/player/player_input_state.h"
#include "vw/gfx/world/entity_guard.h"
#include "vw/gfx/world_grid/world_grid.h"

namespace vw::arena {

class player {
public:
    explicit player(gfx::engine<>& engine, gfx::asset_storage& assets);

    auto update(const gfx::player_input_state& input) -> void;
    auto try_place(float32 voxel_scale) -> void;
    auto toggle_sword() -> void;
    auto handle_attack() -> void;

    [[nodiscard]] auto get_entity() const -> gfx::entity;
    [[nodiscard]] auto has_sword() const -> bool;
    [[nodiscard]] auto is_placed() const -> bool;

private:
    [[nodiscard]] auto create_body_part(
        std::string_view prefab_name, std::string_view part_name
    ) const -> std::unique_ptr<gfx::entity_guard<>>;

    auto setup_animation_fsm() const -> void;

    gfx::engine<>& engine_;
    gfx::asset_storage& assets_;

    std::unique_ptr<gfx::entity_guard<>> root_;
    std::unique_ptr<gfx::entity_guard<>> body_;
    std::unique_ptr<gfx::entity_guard<>> head_;
    std::unique_ptr<gfx::entity_guard<>> hand_right_;
    std::unique_ptr<gfx::entity_guard<>> hand_left_;
    std::unique_ptr<gfx::entity_guard<>> foot_right_;
    std::unique_ptr<gfx::entity_guard<>> foot_left_;
    std::unique_ptr<gfx::entity_guard<>> sword_;

    bool placed_            = false;
    bool need_update_jump_  = false;
    int32 jump_counter_     = 0;
};

}  // namespace vw::arena

#include "player.inl.h"

#endif  // VW_ARENA_PLAYER_H
