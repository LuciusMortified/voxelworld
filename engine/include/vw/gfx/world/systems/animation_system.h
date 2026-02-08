#pragma once

#ifndef VW_GFX_ANIMATION_SYSTEM_H
#define VW_GFX_ANIMATION_SYSTEM_H

#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "vw/core/transform.h"
#include "vw/gfx/animation/animation_clip.h"
#include "vw/gfx/animation/animation_clip_registry.h"
#include "vw/gfx/world/components/animation_component.h"
#include "vw/gfx/world/components/animation_target_component.h"
#include "vw/gfx/world/components/hierarchy_component.h"
#include "vw/gfx/world/components/transform_component.h"
#include "vw/gfx/world/registry.h"

namespace vw::gfx {

template <typename WC>
class world;

template <typename... Cs>
class transform_system;

// Система анимаций - управляет воспроизведением анимаций
// Оптимизирована для работы только с активными анимациями
template <typename... Cs>
class animation_system final {
public:
    using world_type = world<std::tuple<Cs...>>;
    using registry_type = registry<Cs...>;
    using transform_system_type = transform_system<Cs...>;

    explicit animation_system(
        world_type& world,
        registry_type& registry,
        transform_system_type& transform_sys,
        animation_clip_registry& clip_registry
    );

    void update(float32 delta_time);

    class animation_modifier {
    public:
        void play();
        void pause();
        void stop();
        void resume();
        void set_clip(std::shared_ptr<animation_clip> clip);
        void set_clip_by_name(std::string_view name);
        void set_time(float32 time);
        void set_playback_speed(float32 speed);
        void set_loop_mode(animation_loop_mode mode);
        void blend_to(std::shared_ptr<animation_clip> clip, float32 blend_duration);
        void blend_to_by_name(std::string_view name, float32 blend_duration);
        [[nodiscard]] auto get_clip() const -> std::shared_ptr<animation_clip>;
        [[nodiscard]] auto get_state() const -> animation_state;
        [[nodiscard]] auto get_current_time() const -> float32;

    private:
        friend class animation_system;
        animation_modifier(animation_system* system, entity ent, animation_component* component);

        animation_system* system_;
        entity entity_;
        animation_component* component_;
    };

    auto modify(entity ent) -> animation_modifier;

private:
    std::unordered_set<entity> active_entities_;
    std::unordered_map<entity, std::unordered_map<std::string, entity>> target_maps_;
    std::vector<entity> to_remove_;

    void add_active_entity(entity root_ent);
    void remove_active_entity(entity root_ent);
    void build_and_cache_target_map(entity root_ent);
    [[nodiscard]] auto get_cached_target_map(entity root_ent) const
        -> const std::unordered_map<std::string, entity>*;
    void process_animation(entity ent, animation_component& anim_comp, float32 delta_time);
    void apply_animation_to_transform(entity root_ent, const animation_component& anim_comp);
    [[nodiscard]] auto blend_transforms(
        const transform& t1,
        const transform& t2,
        float32 factor
    ) const -> transform;

    world_type* world_;
    registry_type* registry_;
    transform_system_type* transform_system_;
    animation_clip_registry* clip_registry_;
};

// Template specialization для tuple
template <typename... Cs>
struct animation_system_from_tuple;

template <typename... Cs>
struct animation_system_from_tuple<std::tuple<Cs...>> {
    using type = animation_system<Cs...>;
};

}  // namespace vw::gfx

#include "vw/gfx/world/systems/animation_system.inl.h"

#endif  // VW_GFX_ANIMATION_SYSTEM_H
