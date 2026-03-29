#pragma once

#ifndef VW_GFX_ANIMATION_SYSTEM_H
#define VW_GFX_ANIMATION_SYSTEM_H

#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "vw/core/transform.h"
#include "vw/gfx/animation/animation_clip.h"
#include "vw/gfx/animation/animation_clip_registry.h"
#include "vw/gfx/animation/animation_layer.h"
#include "vw/gfx/world/components/animation_player_component.h"
#include "vw/gfx/world/components/animation_target_component.h"
#include "vw/gfx/world/components/hierarchy_component.h"
#include "vw/gfx/world/components/transform_component.h"
#include "vw/gfx/world/entity_registry.h"
#include "vw/gfx/world/world_context.h"

namespace vw::gfx {

template <typename>
class transform_system;

template <typename WC>
class animation_system final {
public:
    using registry_type         = entity_registry_from_tuple<WC>::type;
    using context_type          = world_context<WC>;
    using transform_system_type = transform_system<WC>;

    explicit animation_system(
        context_type& context,
        transform_system_type& transform_sys,
        animation_clip_registry& clip_registry
    );

    void update(float32 delta_time);

    [[nodiscard]] auto get_target_fps() const -> float32;
    void set_target_fps(float32 fps);

    class layer_modifier {
    public:
        void play() const;
        void play(const transition& fade_in) const;
        void pause() const;
        void stop() const;
        void stop(const transition& fade_out) const;
        void clear() const;
        void resume() const;
        void set_time(float32 time) const;
        void set_playback_speed(float32 speed) const;
        void set_loop_mode(animation_loop_mode mode) const;
        void set_fade_in(const transition& t) const;
        void set_fade_out(const transition& t) const;
        void blend_to(
            std::shared_ptr<animation_clip> clip, std::optional<transition> t = std::nullopt
        ) const;
        void blend_to_by_name(std::string_view name, std::optional<transition> t = std::nullopt);

    private:
        friend class animation_system;
        layer_modifier(animation_system* system, entity ent, animation_layer* layer);

        animation_system* system_;
        entity entity_;
        animation_layer* layer_;
    };

    class player_modifier {
    public:
        auto layer(size_t index) -> layer_modifier;
        void apply_pose();
        void rebuild_target_map();

    private:
        friend class animation_system;
        player_modifier(
            animation_system* system, entity ent, animation_player_component* component
        );

        animation_system* system_;
        entity entity_;
        animation_player_component* component_;
    };

    class target_modifier {
    public:
        void set_target_name(std::string name) const;

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
    float32 target_frame_time_      = 1.0f / 120.0f;

    void add_active_entity(entity root_ent);

    void remove_active_entity(entity root_ent);

    void build_and_cache_target_map(entity root_ent);

    [[nodiscard]] auto get_cached_target_map(entity root_ent) const
        -> const std::unordered_map<std::string, entity>*;

    void process_animation(entity ent, animation_player_component& anim_comp, float32 delta_time);

    static void update_layer_time(animation_layer& layer, float32 delta_time);

    void process_layer(animation_layer& layer, float32 delta_time, bool is_base);

    void apply_animation(entity root_ent, const animation_player_component& anim_comp);

    [[nodiscard]] auto compute_layer_transform(
        const animation_layer& layer, const std::string& target_name
    ) const -> std::optional<transform>;

    context_type* context_;
    transform_system_type* transform_system_;
    animation_clip_registry* clip_registry_;
};

}  // namespace vw::gfx

#include "vw/gfx/world/systems/animation_system.inl.h"

#endif  // VW_GFX_ANIMATION_SYSTEM_H
