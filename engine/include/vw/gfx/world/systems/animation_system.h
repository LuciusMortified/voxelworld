#pragma once

#ifndef VW_GFX_ANIMATION_SYSTEM_H
#define VW_GFX_ANIMATION_SYSTEM_H

#include <deque>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "vw/core/transform.h"
#include "vw/gfx/animation/animation_clip.h"
#include "vw/gfx/animation/animation_clip_registry.h"
#include "vw/gfx/world/components/animation_player_component.h"
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

    [[nodiscard]] auto get_target_frame_time() const -> float32;
    void set_target_frame_time(float32 frame_time);

    class player_modifier {
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

    private:
        friend class animation_system;
        player_modifier(animation_system* system, entity ent, animation_player_component* component);

        animation_system* system_;
        entity entity_;
        animation_player_component* component_;
    };

    class target_modifier {
    public:
        void set_target_name(std::string name);

    private:
        friend class animation_system;
        target_modifier(entity ent, animation_target_component* component);

        entity entity_;
        animation_target_component* component_;
    };

    auto modify_player(entity ent) -> player_modifier;
    auto modify_target(entity ent) -> target_modifier;

private:
    std::unordered_set<entity> active_entities_;
    std::unordered_map<entity, std::unordered_map<std::string, entity>> target_maps_;
    std::vector<entity> to_remove_;
    std::deque<entity> to_visit_;
    float32 accumulated_delta_time_ = 0.0f;
    float32 target_frame_time_ = 1.0f / 30.0f;

    void add_active_entity(entity root_ent);
    void remove_active_entity(entity root_ent);
    void build_and_cache_target_map(entity root_ent);
    [[nodiscard]] auto get_cached_target_map(entity root_ent) const
        -> const std::unordered_map<std::string, entity>*;
    void process_animation(entity ent, animation_player_component& anim_comp, float32 delta_time);
    void update_anim_time(animation_player_component& anim_comp, float32 delta_time);
    void apply_animation(entity root_ent, const animation_player_component& anim_comp);

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
