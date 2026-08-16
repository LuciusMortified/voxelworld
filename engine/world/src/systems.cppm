export module vw.world:systems;

import std;

import vw.core;
import vw.ecs;
import :anim;
import :components;
import :grid;
import :index;
import :model;
import :terrain;

export namespace vw::ecs {

class world;

template <typename S, typename C>
concept has_on_add = requires(S& s, entity e) { s.template on_add<C>(e); };

template <typename S, typename C>
concept has_on_remove = requires(S& s, entity e) { s.template on_remove<C>(e); };

template <typename S>
concept has_shutdown = requires(S& s) { s.shutdown(); };

namespace detail {

template <typename C, typename S>
void invoke_on_add(S& system, entity ent) {
    if constexpr (has_on_add<S, C>) {
        system.template on_add<C>(ent);
    }
}

template <typename C, typename S>
void invoke_on_remove(S& system, entity ent) {
    if constexpr (has_on_remove<S, C>) {
        system.template on_remove<C>(ent);
    }
}

template <typename S>
void invoke_shutdown(S& system) {
    if constexpr (has_shutdown<S>) {
        system.shutdown();
    }
}

}  // namespace detail

class hierarchy_system final {
public:
    explicit hierarchy_system(world& w);

    void update(float32 dt);

    class hierarchy_modifier {
    public:
        auto set_parent(entity parent) -> hierarchy_modifier&;
        auto remove_parent() -> hierarchy_modifier&;

    private:
        friend class hierarchy_system;
        hierarchy_modifier(hierarchy_system* system, entity ent);

        hierarchy_system* system_;
        entity entity_;
    };

    void cleanup(entity ent);

    template <typename C>
        requires std::same_as<C, hierarchy_component>
    void on_remove(entity e) {
        cleanup(e);
    }

    [[nodiscard]] auto modify(entity ent) -> hierarchy_modifier;

    [[nodiscard]] auto get_hierarchy_depth(entity ent) const -> std::size_t;

private:
    [[nodiscard]] auto check_hierarchy_cycle(entity parent, entity child) const -> bool;

    world* world_;
};

class transform_system final {
public:
    explicit transform_system(world& w);

    void update(float32 dt);

    class transform_modifier {
    public:
        auto set_transform(const transform& transform) -> transform_modifier&;
        auto set_transform_with_matrix(const transform& transform, const mat4f& local_matrix)
            -> transform_modifier&;
        auto set_position(const vec3f& position) -> transform_modifier&;
        auto set_rotation(const quat& rotation) -> transform_modifier&;
        auto set_rotation_euler(const vec3f& euler) -> transform_modifier&;
        auto set_scale(const vec3f& scale) -> transform_modifier&;
        auto set_origin(const vec3f& origin) -> transform_modifier&;
        auto translate(const vec3f& offset) -> transform_modifier&;
        auto rotate(const vec3f& angles) -> transform_modifier&;
        auto scale(const vec3f& factor) -> transform_modifier&;
        auto mark_world_dirty() -> transform_modifier&;

    private:
        friend class transform_system;
        transform_modifier(transform_system* system, entity ent);

        transform_system* system_;
        entity entity_;
    };

    [[nodiscard]] auto modify(entity ent) -> transform_modifier;

    template <typename C>
        requires(std::same_as<C, transform_component> || std::same_as<C, spatial_component>)
    void on_add(entity e);

private:
    void mark_children_world_dirty(entity ent);
    void update_entity_world_matrix(entity ent, const transform_component& transform_comp);

    world* world_;
    std::vector<std::pair<std::size_t, entity>> sorted_entities_;
};

class model_system {
public:
    explicit model_system(world& w);

    class model_modifier {
    public:
        explicit model_modifier(model_system& system, model_component* component, entity e);

        [[nodiscard]] auto get_model() const -> std::shared_ptr<asset::model>;

        void set_model(std::shared_ptr<asset::model> model_ptr);
        void set_top_brightness(bool enabled);
        void set_voxel(int32 x, int32 y, int32 z, const voxel& v);
        void set_voxel(vec3i pos, const voxel& v);
        void fill(const voxel& v);

    private:
        model_system* system_;
        model_component* component_;
        entity entity_;
    };

    auto modify(entity e) -> model_modifier;
    void update(float32 dt);

    template <typename C>
        requires std::same_as<C, model_component>
    void on_add(entity e);

private:
    world* world_;
};

class light_system final {
public:
    explicit light_system(world& w);

    void update(float32 dt);

    class light_modifier {
    public:
        auto set_color(const vec3f& color) -> light_modifier&;
        auto set_intensity(float32 intensity) -> light_modifier&;
        auto set_range(float32 range) -> light_modifier&;
        auto set_attenuation(float32 constant, float32 linear, float32 quadratic)
            -> light_modifier&;

    private:
        friend class light_system;
        light_modifier(light_system* system, entity ent);

        light_system* system_;
        entity entity_;
    };

    auto modify(entity ent) -> light_modifier;

    template <typename C>
        requires std::same_as<C, light_component>
    void on_add(entity e);

private:
    world* world_;
};

struct voxel_ray_hit {
    entity ent;
    vec3i voxel_pos;
    vec3i empty_pos;
};

class spatial_system {
public:
    explicit spatial_system(world& w);

    void update(float32 dt);

    void query_all(const spatial::frustum& f, std::vector<entity>& result_out,
                   spatial_layer_mask layer_mask = spatial_layer::all) const;

    void query_all(const spatial::ray& r, std::vector<entity>& result_out,
                   spatial_layer_mask layer_mask = spatial_layer::all) const;

    void query_all(const spatial::aabb& bounds, std::vector<entity>& result_out,
                   spatial_layer_mask layer_mask = spatial_layer::all) const;

    void query_all_any(std::span<const spatial::frustum> frustums,
                       std::vector<entity>& result_out) const;

    [[nodiscard]] auto voxel_ray_cast(const spatial::ray& r, std::vector<entity>& candidates,
                                      spatial_layer_mask layer_mask = spatial_layer::all) const
        -> std::optional<voxel_ray_hit>;

    void cleanup(entity ent);

    template <typename C>
        requires std::same_as<C, spatial_component>
    void on_remove(entity e) {
        cleanup(e);
    }

    class spatial_modifier {
    public:
        auto set_layer(spatial_layer_mask layer) -> spatial_modifier&;

    private:
        friend class spatial_system;
        spatial_modifier(spatial_system* system, entity ent);

        spatial_system* system_;
        entity entity_;
    };

    auto modify(entity ent) -> spatial_modifier;

private:
    void update_entity(entity ent);
    auto calculate_aabb_from_model(entity ent, const model_component& model_comp,
                                   const transform_component& transform_comp) const
        -> spatial::aabb;
    static auto expand_aabb_for_fat(const spatial::aabb& bounds) -> spatial::aabb;

    world* world_;
    dynamic_aabb_tree tree_;
};

struct physics_stats {
    float32 step_ms             = 0.0F;
    float32 voxel_collision_ms  = 0.0F;
    float32 entity_collision_ms = 0.0F;
    float32 entity_query_ms     = 0.0F;
    float32 entity_resolve_ms   = 0.0F;
    int32 step_count            = 0;
    int32 entity_query_results  = 0;
};

class physics_system final {
public:
    static constexpr int32 max_collision_iterations = 4;
    static constexpr float32 fixed_dt               = 1.0F / 60.0F;
    static constexpr int32 max_steps_per_frame      = 5;

    explicit physics_system(world& w);

    void set_gravity(float32 g);
    [[nodiscard]] auto get_gravity() const -> float32;

    void update(float32 delta_time);
    [[nodiscard]] auto get_stats() const -> const physics_stats&;

    class rigid_body_modifier {
    public:
        auto set_velocity(const vec3f& vel) -> rigid_body_modifier&;
        auto set_gravity_scale(float32 scale) -> rigid_body_modifier&;
        auto add_impulse(const vec3f& impulse) -> rigid_body_modifier&;
        auto add_external_impulse(const vec3f& impulse) -> rigid_body_modifier&;
        auto set_drag(float32 drag) -> rigid_body_modifier&;

    private:
        friend class physics_system;
        rigid_body_modifier(physics_system* system, entity ent);

        physics_system* system_;
        entity entity_;
    };

    class collider_modifier {
    public:
        auto set_extents(const vec3f& ext) -> collider_modifier&;
        auto set_offset(const vec3f& offset) -> collider_modifier&;

    private:
        friend class physics_system;
        collider_modifier(physics_system* system, entity ent);

        physics_system* system_;
        entity entity_;
    };

    auto modify(entity ent) -> rigid_body_modifier;
    auto modify_collider(entity ent) -> collider_modifier;

private:
    void step(float32 dt);
    [[nodiscard]] auto are_chunks_loaded(const vec3f& position, const vec3f& extents) const -> bool;

    struct collision_result {
        vec3f resolved_position;
        bool grounded = false;
    };

    [[nodiscard]] auto resolve_box_voxel(vec3f center, const vec3f& half_extents,
                                         vec3f& velocity) const -> collision_result;

    void resolve_entity_collisions(entity ent, vec3f& position, vec3f& velocity,
                                   const vec3f& half_extents, const vec3f& offset);

    world* world_;
    float32 gravity_          = -300.0F;
    float32 accumulated_time_ = 0.0F;
    physics_stats stats_;
    std::vector<entity> entity_query_cache_;
};

class character_controller_system final {
public:
    explicit character_controller_system(world& w);

    void update(float32 delta_time);

    class controller_modifier {
    public:
        auto set_move_input(const vec3f& input) -> controller_modifier&;
        auto set_facing_direction(const vec3f& direction) -> controller_modifier&;
        auto set_move_speed(float32 speed) -> controller_modifier&;
        auto set_jump_impulse(float32 impulse) -> controller_modifier&;
        auto set_rotation_speed(float32 speed) -> controller_modifier&;
        auto request_jump() -> controller_modifier&;

    private:
        friend class character_controller_system;
        controller_modifier(character_controller_system* system, entity ent);

        character_controller_system* system_;
        entity entity_;
    };

    auto modify(entity ent) -> controller_modifier;

private:
    world* world_;
};

class socket_system final {
public:
    explicit socket_system(world& w);

    class socket_modifier {
    public:
        auto attach(const std::string& socket_name, entity child) -> socket_modifier&;
        auto detach(const std::string& socket_name) -> socket_modifier&;
        auto add_socket(const std::string& name, const vec3f& position = {},
                        const quat& rotation = {}, const vec3f& scale = vec3f{1, 1, 1})
            -> socket_modifier&;
        auto remove_socket(const std::string& name) -> socket_modifier&;

    private:
        friend class socket_system;
        socket_modifier(socket_system* system, entity ent);

        socket_system* system_;
        entity entity_;
    };

    auto modify(entity ent) -> socket_modifier;
    void cleanup(entity ent);
    void update(float32 dt);

    template <typename C>
        requires std::same_as<C, socket_component>
    void on_remove(entity e) {
        cleanup(e);
    }

private:
    world* world_;
};

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

class animation_fsm_system final {
public:
    explicit animation_fsm_system(world& w);

    void update(float32 dt);

    class modifier {
    public:
        void add_machine(std::size_t index, asset::animation_fsm machine) const;
        void fire_trigger(std::string_view name) const;

    private:
        friend class animation_fsm_system;
        explicit modifier(animation_fsm_component* component);

        animation_fsm_component* component_;
    };

    auto modify(entity ent) -> modifier;

private:
    world* world_;
};

struct world_grid_system_stats {
    float32 integrate_ms       = 0.0F;
    float32 stage_ms           = 0.0F;
    float32 boundary_from_ms   = 0.0F;
    float32 chunk_create_ms    = 0.0F;
    float32 request_columns_ms = 0.0F;
    float32 rebuild_active_ms  = 0.0F;
    float32 unload_ms          = 0.0F;
    uint32 active_count        = 0;
    uint32 pending_count       = 0;
    uint32 loaded_count        = 0;

    // Of the loaded ones, those with an entity behind them. Solid rock and open
    // air have nothing to draw and get neither entity nor mesh.
    uint32 drawn_count = 0;

    // Generated, held back until its four neighbours are there too.
    uint32 staged_count = 0;
};

class world_grid_system {
public:
    explicit world_grid_system(world& w);
    ~world_grid_system();

    world_grid_system(const world_grid_system&)                    = delete;
    auto operator=(const world_grid_system&) -> world_grid_system& = delete;
    world_grid_system(world_grid_system&&) noexcept;
    auto operator=(world_grid_system&&) noexcept -> world_grid_system&;

    void set_grid(std::unique_ptr<world_grid> grid);
    void set_loader(std::unique_ptr<chunk_loader> loader);

    [[nodiscard]] auto grid() -> world_grid*;
    [[nodiscard]] auto grid() const -> const world_grid*;
    [[nodiscard]] auto loader() -> chunk_loader*;
    [[nodiscard]] auto loader() const -> const chunk_loader*;
    [[nodiscard]] auto has_grid() const -> bool;
    [[nodiscard]] auto has_loader() const -> bool;

    void update(float32 dt);
    void shutdown();

    [[nodiscard]] auto get_stats() const -> const world_grid_system_stats&;
    [[nodiscard]] auto get_loader_stats() const -> column_gen_stats;

    class view_modifier {
    public:
        auto set_view_distance(uint32 distance) -> view_modifier&;

    private:
        friend class world_grid_system;
        view_modifier(world_grid_system* system, entity ent);

        world_grid_system* system_;
        entity entity_;
    };

    auto modify_view(entity ent) -> view_modifier;

private:
    // A column waits in staging until its four horizontal neighbours have been
    // generated too. Only then are its chunks put in the grid, with every
    // boundary already known -- so each chunk is meshed once instead of once
    // per neighbour that turns up after it.
    //
    // The active set is therefore one column wider than the view distance: the
    // outermost ring exists to complete the ring inside it and is never placed.
    static constexpr int32 apron_columns = 1;

    auto process_dirty_entity_(entity ent) -> bool;
    auto process_dirty_entities_() -> bool;
    void stage_completed_columns_();
    void integrate_completed_columns_();
    [[nodiscard]] auto column_available_(vec2i coord) const -> bool;
    [[nodiscard]] auto column_ready_(vec2i coord) const -> bool;
    [[nodiscard]] auto within_draw_(vec2i coord) const -> bool;
    [[nodiscard]] auto model_at_(vec3i chunk_coord) const -> asset::model*;
    void queue_if_ready_(vec2i coord);
    void demote_column_(vec2i coord);
    void dispatch_column_requests_();
    void update_grid_stats_();
    auto rebuild_active_set_() -> vec2i;
    void unload_inactive_columns_();
    void rebuild_pending_requests_(vec2i camera_column);
    void clear_grid_transient_state_();
    void clear_loader_transient_state_();

    world* world_;
    std::unique_ptr<world_grid> grid_;
    std::unique_ptr<chunk_loader> loader_;
    std::unordered_set<vec2i> active_columns_;
    std::unordered_set<vec2i> pending_active_columns_;
    std::vector<vec2i> pending_requests_;
    std::unordered_map<vec2i, std::unique_ptr<gen_column>> staged_columns_;
    std::vector<vec2i> ready_columns_;

    // The apron is generated but never drawn, so it must not keep drawn columns
    // alive behind the camera: staging reaches one column further than this,
    // placed columns are let go at it.
    vec2i camera_column_{};
    int32 draw_distance_ = 0;

    world_grid_system_stats stats_;
};

}  // namespace vw::ecs
