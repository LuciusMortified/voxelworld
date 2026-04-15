#pragma once

namespace vw::arena {

inline dummy_enemy::dummy_enemy(gfx::engine<>& engine, const vec2f& spawn_xz)
    : engine_{engine}
    , spawn_xz_{spawn_xz} {
    auto& world            = engine_.get_world();
    auto& transform_sys = world.template get_system<gfx::transform_system>();
    auto& physics_sys = world.template get_system<gfx::physics_system>();
    auto& model_sys = world.template get_system<gfx::model_system>();

    root_ = std::make_unique<gfx::entity_guard<gfx::base_world_def>>(world.get_context());
    root_->with<gfx::hierarchy_component>();
    root_->with<gfx::transform_component>();
    root_->with<gfx::spatial_component>();
    root_->with<gfx::rigid_body_component>();
    root_->with<gfx::box_collider_component>();
    root_->with<gfx::model_component>();

    const auto ent = root_->get_entity();

    transform_sys.modify(ent)
        .set_position({spawn_xz_.x, 500.0f, spawn_xz_.y})
        .set_origin({-8.0f, -16.0f, -8.0f});

    physics_sys.modify_collider(ent)
        .set_extents({16.0f, 32.0f, 16.0f})
        .set_offset({0.0f, 0.0f, 0.0f});

    world.template get_system<gfx::spatial_system>().modify(ent).set_layer(gfx::spatial_layer::character);

    model_sys.modify(ent).set_model(create_model());
}

inline auto dummy_enemy::try_place() -> void {
    if (placed_) {
        return;
    }

    const auto grid = engine_.get_world().get_world_grid();
    const auto vs   = grid->voxel_scale();
    const auto surface = grid->get_surface_y(
        static_cast<int32>(spawn_xz_.x / vs),
        static_cast<int32>(spawn_xz_.y / vs)
    );
    if (!surface) {
        return;
    }

    float32 spawn_y = (static_cast<float32>(*surface) + 6.0f) * vs;

    engine_.get_world()
        .template get_system<gfx::transform_system>()
        .modify(root_->get_entity())
        .set_position({spawn_xz_.x, spawn_y, spawn_xz_.y});

    placed_ = true;
}

inline auto dummy_enemy::get_entity() const -> gfx::entity {
    return root_->get_entity();
}

inline auto dummy_enemy::is_placed() const -> bool {
    return placed_;
}

inline auto dummy_enemy::create_model() -> std::shared_ptr<gfx::model> {
    auto& model_reg = engine_.get_world().template get_resource<gfx::model_registry>();
    auto model = model_reg.create_unnamed(16, 32, 16);
    model->fill(voxel{blocks::red_3});
    return model;
}

}  // namespace vw::arena