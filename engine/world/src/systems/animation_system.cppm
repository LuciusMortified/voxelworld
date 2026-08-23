export module vw.world:systems.animation;

import std;

import vw.core;
import vw.ecs;
import :anim;
import :components;
import :grid;
import :index;
import :model;
import :light;
import :terrain;

export namespace vw::ecs {

class world;

class animation_system final {
public:
    explicit animation_system(world& w);

    void update(float32 delta_time);

    [[nodiscard]] auto get_target_fps() const -> float32;
    void set_target_fps(float32 fps);

    class layer_modifier {
    public:
        void play() const;
        void play(const asset::transition& fade_in) const;
        void pause() const;
        void stop() const;
        void stop(const asset::transition& fade_out) const;
        void clear() const;
        void resume() const;
        void set_time(float32 time) const;
        void set_playback_speed(float32 speed) const;
        void set_loop_mode(asset::animation_loop_mode mode) const;
        void set_fade_in(const asset::transition& t) const;
        void set_fade_out(const asset::transition& t) const;
        void blend_to(std::shared_ptr<asset::animation_clip> clip,
                      std::optional<asset::transition> t = std::nullopt) const;
        void blend_to_by_name(std::string_view name,
                              std::optional<asset::transition> t = std::nullopt);

    private:
        friend class animation_system;
        layer_modifier(animation_system* system, entity ent, asset::animation_layer* layer);

        animation_system* system_;
        entity entity_;
        asset::animation_layer* layer_;
    };

    class player_modifier {
    public:
        void add_layer(std::size_t index) const;
        auto layer(std::size_t index) -> layer_modifier;
        void apply_pose() const;
        void rebuild_target_map() const;

    private:
        friend class animation_system;
        player_modifier(animation_system* system, entity ent,
                        animation_player_component* component);

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
    void add_active_entity(entity root_ent);
    void remove_active_entity(entity root_ent);
    void build_and_cache_target_map(entity root_ent);

    [[nodiscard]] auto get_cached_target_map(entity root_ent) const
        -> const std::unordered_map<std::string, entity>*;

    void process_animation(entity ent, animation_player_component& anim_comp, float32 delta_time);

    static void update_layer_time(asset::animation_layer& layer, float32 delta_time);

    void process_layer(asset::animation_layer& layer, float32 delta_time, bool is_base);
    void apply_animation(entity root_ent, const animation_player_component& anim_comp);

    [[nodiscard]] auto compute_layer_transform(const asset::animation_layer& layer,
                                               const std::string& target_name,
                                               const transform& rest) const
        -> std::optional<transform>;

    static auto merge_with_rest(const transform& anim, const asset::animation_track& track,
                                const transform& rest) -> transform;

    std::unordered_set<entity> active_entities_;
    std::unordered_map<entity, std::unordered_map<std::string, entity>> target_maps_;
    std::vector<entity> to_remove_;
    std::deque<entity> to_visit_;
    float32 accumulated_delta_time_ = 0.0F;
    float32 target_frame_time_      = 1.0F / 120.0F;

    world* world_;
};

}  // namespace vw::ecs
