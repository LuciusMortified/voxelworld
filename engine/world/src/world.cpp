#include "vw/ecs/world.h"

#include <utility>

namespace vw::ecs {
namespace {

template <typename Tuple, std::size_t... Is>
auto make_systems(world& w, std::index_sequence<Is...> /*unused*/) -> Tuple {
    return Tuple{std::tuple_element_t<Is, Tuple>(w)...};
}

}  // namespace

world::world()
    : systems_{make_systems<systems>(
          *this, std::make_index_sequence<std::tuple_size_v<systems>>{})} {}

world::~world() {
    std::apply([](auto&... s) { (detail::invoke_shutdown(s), ...); }, systems_);

    for (auto ent : registry_.alive_entities()) {
        destroy(ent);
    }
}

void world::update(float32 delta_time) {
    std::apply([delta_time](auto&... s) { (s.update(delta_time), ...); }, systems_);
}

void world::clear_changed() {
    registry_.clear_changed();
}

auto world::create() -> modifier {
    return modifier(*this, registry_.create());
}

auto world::modify(entity ent) -> modifier {
    return modifier(*this, ent);
}

void world::destroy(entity ent) noexcept {
    detach_components_(ent);
    registry_.destroy(ent);
}

auto world::batch_create(uint32 count) -> batch_modifier {
    return batch_modifier(*this, registry_.batch_create(count));
}

auto world::batch_modify(std::vector<entity> entities) -> batch_modifier {
    return batch_modifier(*this, std::move(entities));
}

void world::batch_destroy(const std::vector<entity>& entities) noexcept {
    for (auto ent : entities) {
        detach_components_(ent);
    }
    registry_.batch_destroy(entities);
}

auto world::registry() -> ecs::registry& {
    return registry_;
}

auto world::destroyed() const -> const std::vector<entity>& {
    return registry_.destroyed();
}

void world::detach_components_(entity ent) noexcept {
    for (uint32 id = 0; id < registry_.pool_count(); ++id) {
        const auto* pool = registry_.try_pool(id);
        if (pool == nullptr || !pool->has(ent)) {
            continue;
        }
        if (id < remove_hooks_.size() && remove_hooks_[id] != nullptr) {
            remove_hooks_[id](*this, ent);
        }
    }
}

}  // namespace vw::ecs
