module vw.world;

import std;
import vw.core;

namespace vw::ecs {

light_system::light_system(world& w)
    : world_(&w) {}

template <typename C>
    requires std::same_as<C, light_component>
void light_system::on_add(entity e) {
    world_->registry().request_change<light_component>(e);
}

// Извещение больше никто не потребляет: gfx::light_buffer вместо этого
// перестраивается из мира каждый кадр. И не потому, что движущийся источник
// ускользает от набора изменений — не ускользает: world.cpp вешает зависимость
// изменения на transform_component, а эта система идёт после transform_system,
// поэтому несомый факел помечается изменённым в том же кадре, в котором двигается.
//
// Дело в отсеве, который на компонент не повесить. Буфер обрезается по фрустуму, а
// фрустум следует за камерой, и она поворачивается, не меняя ни одного компонента.
//
// Опустошать набор всё равно надо: модификаторы продолжают его наполнять, а
// набор, который никто не чистит, только растёт.
void light_system::update(float32 /*dt*/) {
    auto& reg       = world_->registry();
    auto& requested = reg.requested<light_component>();
    if (requested.empty()) {
        return;
    }

    for (entity ent : requested) {
        reg.notify_changed<light_component>(ent);
    }

    reg.clear_requested<light_component>();
}

light_system::light_modifier::light_modifier(
    light_system* system, entity ent
)
    : system_(system), entity_(ent) {}

auto light_system::modify(entity ent) -> light_modifier {
    return light_modifier(this, ent);
}

auto light_system::light_modifier::set_color(
    const vec3f& color
) -> light_modifier& {
    auto& reg = system_->world_->registry();
    if (!reg.has<light_component>(entity_)) {
        return *this;
    }
    auto& comp = reg.get<light_component>(entity_);
    comp.color_ = color;
    reg.request_change<light_component>(entity_);
    return *this;
}

auto light_system::light_modifier::set_intensity(
    float32 intensity
) -> light_modifier& {
    auto& reg = system_->world_->registry();
    if (!reg.has<light_component>(entity_)) {
        return *this;
    }
    auto& comp = reg.get<light_component>(entity_);
    comp.intensity_ = intensity;
    reg.request_change<light_component>(entity_);
    return *this;
}

auto light_system::light_modifier::set_range(
    float32 range
) -> light_modifier& {
    auto& reg = system_->world_->registry();
    if (!reg.has<light_component>(entity_)) {
        return *this;
    }
    auto& comp = reg.get<light_component>(entity_);
    comp.range_ = range;
    reg.request_change<light_component>(entity_);
    return *this;
}

template void light_system::on_add<light_component>(entity);

}  // namespace vw::ecs