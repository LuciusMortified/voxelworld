#pragma once

#ifndef VW_ARENA_DUMMY_ENEMY_H
#define VW_ARENA_DUMMY_ENEMY_H

#include <memory>

#include <vw/core.h>
#include <vw/gfx.h>

#include "vw/gfx/world/entity_guard.h"

namespace vw::arena {

class dummy_enemy {
public:
    explicit dummy_enemy(gfx::engine<>& engine, const vec2f& spawn_xz);

    auto try_place() -> void;

    [[nodiscard]] auto get_entity() const -> gfx::entity;
    [[nodiscard]] auto is_placed() const -> bool;

private:
    auto create_model() -> std::shared_ptr<asset::model>;

    gfx::engine<>& engine_;
    std::unique_ptr<gfx::entity_guard<gfx::base_world_def>> root_;
    vec2f spawn_xz_;
    bool placed_ = false;
};

}  // namespace vw::arena

#include "dummy_enemy.inl.h"

#endif  // VW_ARENA_DUMMY_ENEMY_H