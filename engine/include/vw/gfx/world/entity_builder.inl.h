#pragma once

#ifndef VW_GFX_ENTITY_BUILDER_INL_H
#define VW_GFX_ENTITY_BUILDER_INL_H

namespace vw::gfx {

template <typename WC>
entity_builder<WC>::entity_builder(
    world_type& world
)
    : world_{&world}, ent_{world.create_entity()}, archetype_{} {}

template <typename WC>
template <typename C>
auto entity_builder<WC>::with() -> entity_builder& {
    world_->template add_component<C>(ent_);
    archetype_.template add<C>();

    return *this;
}

template <typename WC>
auto entity_builder<WC>::release() -> entity {
    return ent_;
}

template <typename WC>
auto entity_builder<WC>::release_guard() -> std::unique_ptr<entity_guard_type> {
    return std::make_unique<entity_guard_type>(*world_, ent_, archetype_);
}

}  // namespace vw::gfx

#endif  // VW_GFX_ENTITY_BUILDER_INL_H
