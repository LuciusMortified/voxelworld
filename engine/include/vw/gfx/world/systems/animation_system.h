#pragma once

#ifndef VW_GFX_ANIMATION_SYSTEM_H
#define VW_GFX_ANIMATION_SYSTEM_H

#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "vw/asset/animation/animation_clip.h"
#include "vw/asset/animation/animation_clip_registry.h"
#include "vw/asset/animation/animation_layer.h"
#include "vw/core/transform.h"
#include "vw/gfx/world/components/animation_player_component.h"
#include "vw/gfx/world/components/animation_target_component.h"
#include "vw/gfx/world/components/hierarchy_component.h"
#include "vw/gfx/world/components/transform_component.h"
#include "vw/gfx/world/entity_registry.h"
#include "vw/gfx/world/system_trait.h"

namespace vw::gfx {


template <typename>
class transform_system;

template <typename>
class world;

template <typename WD>
class animation_system final {
public:
    using components    = typename WD::components;
    using registry_type = typename entity_registry_from_tuple<components>::type;
    using world_type    = world<WD>;

    explicit animation_system(world_type& w);

    void update(float32 delta_time);

    [[nodiscard]] auto get_target_fps() const -> float32;
    void set_target_fps(float32 fps);

    class layer_modifier {
    public:
        void play() const;
        void play(const vw::asset::transition& fade_in) const;
        void pause() const;
        void stop() const;
        void stop(const vw::asset::transition& fade_out) const;
        void clear() const;
        void resume() const;
        void set_time(float32 time) const;
        void set_playback_speed(float32 speed) const;
        void set_loop_mode(vw::asset::animation_loop_mode mode) const;
        void set_fade_in(const vw::asset::transition& t) const;
        void set_fade_out(const vw::asset::transition& t) const;
        void blend_to(
            std::shared_ptr<vw::asset::animation_clip> clip, std::optional<vw::asset::transition> t = std::nullopt
        ) const;
        void blend_to_by_name(std::string_view name, std::optional<vw::asset::transition> t = std::nullopt);

    private:
        friend class animation_system;
        layer_modifier(animation_system* system, entity ent, vw::asset::animation_layer* layer);

        animation_system* system_;
        entity entity_;
        vw::asset::animation_layer* layer_;
    };

    class player_modifier {
    public:
        void add_layer(size_t index) const;
        auto layer(size_t index) -> layer_modifier;
        void apply_pose() const;
        void rebuild_target_map() const;

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
        void set_rest_transform(const transform& rest) const;

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

    static void update_layer_time(asset::animation_layer& layer, float32 delta_time);

    void process_layer(asset::animation_layer& layer, float32 delta_time, bool is_base);

    void apply_animation(entity root_ent, const animation_player_component& anim_comp);

    [[nodiscard]] auto compute_layer_transform(
        const asset::animation_layer& layer, const std::string& target_name, const transform& rest
    ) const -> std::optional<transform>;

    static auto merge_with_rest(
        const transform& anim, const asset::animation_track& track, const transform& rest
    ) -> transform;

    world_type* world_;
};

}  // namespace vw::gfx

template <>
struct vw::gfx::system_trait<vw::gfx::animation_system> {
    using components = std::tuple<
        animation_player_component,
        animation_target_component>;
    using resources  = std::tuple<asset::animation_clip_registry>;
};

#include "vw/gfx/world/systems/animation_system.inl.h"

#endif  // VW_GFX_ANIMATION_SYSTEM_H
