#pragma once

#include "vw/gfx/world/serializers/vox_parser_plain.h"

namespace vw::arena {

inline player::player(
    gfx::engine<>& engine, gfx::asset_storage& assets
)
    : engine_{engine}, assets_{assets} {
    auto& world            = engine_.get_world();
    auto& transform_system = world.get_transform_system();
    auto& physics_system   = world.get_physics_system();

    root_ = std::make_unique<gfx::entity_guard<>>(world);
    root_->with<gfx::hierarchy_component>();
    root_->with<gfx::transform_component>();
    root_->with<gfx::spatial_component>();
    root_->with<gfx::rigid_body_component>();
    root_->with<gfx::box_collider_component>();
    root_->with<gfx::character_controller_component>();
    root_->with<gfx::movement_intent_component>();
    root_->with<gfx::world_view_component>();
    root_->with<gfx::animation_player_component>();
    root_->with<gfx::animation_fsm_component>();

    const auto ent = root_->get_entity();

    transform_system.modify(ent)
        .set_position({0.0f, 500.0f, 0.0f})
        .set_origin({-6.0f, -6.0f, -6.0f});

    physics_system.modify_collider(ent)
        .set_extents({12.0f, 28.0f, 12.0f})
        .set_offset({0.0f, 2.0f, 0.0f});

    world.get_spatial_system().modify(ent).set_layer(gfx::spatial_layer::character);

    body_       = create_body_part("m_human", "body");
    head_       = create_body_part("m_human", "head");
    hand_right_ = create_body_part("m_human", "hand_right");
    hand_left_  = create_body_part("m_human", "hand_left");
    foot_right_ = create_body_part("m_human", "foot_right");
    foot_left_  = create_body_part("m_human", "foot_left");

    setup_animation_fsm();
}

inline auto player::update(
    const gfx::player_input_state& input
) -> void {
    const auto ent = root_->get_entity();
    auto& world    = engine_.get_world();
    const auto& rb = world.get_component<gfx::rigid_body_component>(ent);

    if (input.jump_requested && rb.is_grounded()) {
        need_update_jump_ = true;
    }

    if (need_update_jump_ && !rb.is_grounded()) {
        jump_counter_     = (jump_counter_ + 1) % 2;
        need_update_jump_ = false;
    }
}

inline auto player::try_place(
    float32 voxel_scale
) -> void {
    if (placed_) {
        return;
    }

    const auto grid    = engine_.get_world().get_world_grid();
    const auto surface = grid->get_surface_y(0, 0);
    if (!surface) {
        return;
    }

    float32 spawn_y = (static_cast<float32>(*surface) + 6.0f) * voxel_scale;

    engine_.get_world()
        .get_transform_system()
        .modify(root_->get_entity())
        .set_position({0.0f, spawn_y, 0.0f});

    placed_ = true;
}

inline auto player::toggle_sword() -> void {
    if (!hand_right_) {
        return;
    }

    auto& world         = engine_.get_world();
    const auto hand_ent = hand_right_->get_entity();

    if (sword_) {
        world.get_socket_system().modify(hand_ent).detach("hand_right");
        sword_ = nullptr;
    } else {
        sword_ = std::make_unique<gfx::entity_guard<>>(world);
        sword_->with<gfx::hierarchy_component>();
        sword_->with<gfx::transform_component>();
        sword_->with<gfx::spatial_component>();
        sword_->with<gfx::model_component>();

        const auto sword_ent = sword_->get_entity();
        const auto& ent_data = assets_.get_entity("m_sword", "root");

        world.get_transform_system().modify(sword_ent).set_origin(ent_data.origin);
        world.get_model_system().modify(sword_ent).set_model(assets_.get_model("m_sword", "root"));
        world.get_socket_system().modify(hand_ent).attach("hand_right", sword_ent);
        world.get_spatial_system().modify(sword_ent).set_layer(gfx::spatial_layer::character);
    }
}

inline auto player::handle_attack() -> void {
    if (!sword_) {
        return;
    }

    const auto ent = root_->get_entity();
    auto& world    = engine_.get_world();

    const auto& anim_player = world.get_component<gfx::animation_player_component>(ent);
    const auto& layer       = anim_player.get_layer(1);

    if (!layer.is_active()) {
        world.get_animation_fsm_system().modify(ent).fire_trigger("attack");
    }
}

inline auto player::get_entity() const -> gfx::entity {
    return root_->get_entity();
}

inline auto player::has_sword() const -> bool {
    return sword_ != nullptr;
}

inline auto player::is_placed() const -> bool {
    return placed_;
}

inline auto player::create_body_part(
    std::string_view prefab_name, std::string_view part_name
) const -> std::unique_ptr<gfx::entity_guard<>> {
    auto& world            = engine_.get_world();
    auto& hierarchy_system = world.get_hierarchy_system();
    auto& transform_system = world.get_transform_system();
    auto& model_system     = world.get_model_system();
    auto& animation_system = world.get_animation_system();

    auto guard = std::make_unique<gfx::entity_guard<>>(world);
    guard->with<gfx::hierarchy_component>();
    guard->with<gfx::transform_component>();
    guard->with<gfx::spatial_component>();
    guard->with<gfx::model_component>();
    guard->with<gfx::animation_target_component>();

    const auto& ent_data = assets_.get_entity(prefab_name, part_name);
    if (ent_data.has_sockets) {
        guard->with<gfx::socket_component>();
    }

    const auto ent = guard->get_entity();

    hierarchy_system.modify(ent).set_parent(root_->get_entity());

    transform_system.modify(ent)
        .set_position(ent_data.position)
        .set_rotation_euler(ent_data.rotation)
        .set_scale(ent_data.scale)
        .set_origin(ent_data.origin);

    const auto model = assets_.get_model(prefab_name, part_name);
    model_system.modify(ent).set_model(model);

    animation_system.modify_target(ent).set_target_name(ent_data.name);

    if (ent_data.has_sockets) {
        auto& socket_system = world.get_socket_system();
        for (const auto& sp : ent_data.sockets) {
            socket_system.modify(ent).add_socket(
                sp.name, sp.position, math::euler_to_quat(sp.rotation), sp.scale
            );
        }
    }

    return guard;
}

inline auto player::setup_animation_fsm() const -> void {
    const auto ent = root_->get_entity();
    auto& world    = engine_.get_world();

    auto is_walk = [&, ent]() -> bool {
        const auto& move = world.get_component<gfx::movement_intent_component>(ent);
        const auto& v    = move.get_wish_velocity();
        return v.x != 0.0f || v.z != 0.0f;
    };
    auto is_idle = [&, ent]() -> bool {
        const auto& move = world.get_component<gfx::movement_intent_component>(ent);
        const auto& v    = move.get_wish_velocity();
        return v.x == 0.0f && v.z == 0.0f;
    };
    auto is_jump_left = [&, ent]() -> bool {
        const auto& rb = world.get_component<gfx::rigid_body_component>(ent);
        return jump_counter_ == 0 && !rb.is_grounded();
    };
    auto is_jump_right = [&, ent]() -> bool {
        const auto& rb = world.get_component<gfx::rigid_body_component>(ent);
        return jump_counter_ == 1 && !rb.is_grounded();
    };
    auto is_grounded   = [&, ent]() -> bool {
        const auto& rb = world.get_component<gfx::rigid_body_component>(ent);
        return rb.is_grounded();
    };

    constexpr gfx::transition blend_fast{.duration = 0.15f};
    constexpr gfx::transition blend_normal{.duration = 0.25f};
    constexpr gfx::transition blend_slow{.duration = 0.35f};

    gfx::animation_fsm fsm_movement;

    fsm_movement.add_state({
        .name      = "idle",
        .clip      = assets_.get_clip("a_idle"),
        .loop_mode = gfx::animation_loop_mode::loop,
        .transitions =
            {
                {.target_state = "walk", .condition = is_walk, .blend = blend_fast},
                {
                    .target_state     = "jump_left",
                    .condition        = is_jump_left,
                    .blend            = blend_fast,
                    .wait_until_blend = true,
                },
                {
                    .target_state     = "jump_right",
                    .condition        = is_jump_right,
                    .blend            = blend_fast,
                    .wait_until_blend = true,
                },
            },
    });

    fsm_movement.add_state({
        .name           = "walk",
        .clip           = assets_.get_clip("a_walk"),
        .loop_mode      = gfx::animation_loop_mode::loop,
        .playback_speed = 1.25f,
        .transitions =
            {
                {.target_state = "idle", .condition = is_idle, .blend = blend_fast},
                {
                    .target_state = "jump_left",
                    .condition    = is_jump_left,
                    .blend        = blend_fast,
                },
                {
                    .target_state = "jump_right",
                    .condition    = is_jump_right,
                    .blend        = blend_fast,
                },
            },
    });

    fsm_movement.add_state({
        .name           = "jump_right",
        .clip           = assets_.get_clip("a_jump_right"),
        .loop_mode      = gfx::animation_loop_mode::loop,
        .playback_speed = 1.0f,
        .transitions =
            {
                {
                    .target_state     = "idle",
                    .condition        = is_grounded,
                    .blend            = blend_normal,
                    .wait_until_blend = true,
                },
            },
    });

    fsm_movement.add_state({
        .name           = "jump_left",
        .clip           = assets_.get_clip("a_jump_left"),
        .loop_mode      = gfx::animation_loop_mode::loop,
        .playback_speed = 1.0f,
        .transitions =
            {
                {
                    .target_state     = "idle",
                    .condition        = is_grounded,
                    .blend            = blend_normal,
                    .wait_until_blend = true,
                },
            },
    });

    fsm_movement.set_entry_state("idle");

    world.get_animation_fsm_system().modify(ent).add_machine(0, std::move(fsm_movement));

    gfx::animation_fsm fsm_action;

    fsm_action.add_state({
        .name = "none",
        .transitions =
            {
                {
                    .target_state = "sword_attack",
                    .trigger_name = "attack",
                    .blend        = blend_fast,
                },
            },
    });

    fsm_action.add_state({
        .name            = "sword_attack",
        .clip            = assets_.get_clip("a_sword_attack"),
        .loop_mode       = gfx::animation_loop_mode::once,
        .layer_blend_in  = blend_normal,
        .layer_blend_out = blend_slow,
        .transitions =
            {
                {
                    .target_state     = "none",
                    .condition        = []() -> bool { return true; },
                    .wait_until_end   = true,
                    .wait_until_blend = true,
                },
            },
    });

    fsm_action.set_entry_state("none");

    world.get_animation_fsm_system().modify(ent).add_machine(1, std::move(fsm_action));
}

}  // namespace vw::arena
