export module vw.world;

import std;

export import :model;
export import :anim;
export import :serial;
export import :index;
export import :components;
export import :terrain;
export import :sky_light;
export import :grid;
export import :systems;

import vw.core;
import vw.ecs;

export namespace vw::ecs {

// Owns the registry, the systems and the shared asset registries, and keeps
// them in step: adding or removing a component notifies every system that
// cares about that type.
class world final {
public:
    using systems = std::tuple<hierarchy_system, character_controller_system, animation_fsm_system,
                               physics_system, transform_system, model_system, spatial_system,
                               light_system, socket_system, world_grid_system, animation_system>;

    using resources = std::tuple<asset::model_registry, asset::animation_clip_registry>;

    class modifier {
    public:
        modifier(world& w, entity ent) : world_{&w}, ent_{ent} {}

        template <typename C>
        auto with(C&& value = {}) -> modifier& {
            world_->add_component_<C>(ent_, std::forward<C>(value));
            return *this;
        }

        template <typename C>
        auto without() -> modifier& {
            world_->remove_component_<C>(ent_);
            return *this;
        }

        [[nodiscard]] auto get_entity() const -> entity {
            return ent_;
        }

    private:
        world* world_;
        entity ent_;
    };

    class batch_modifier {
    public:
        batch_modifier(world& w, std::vector<entity> entities)
            : world_{&w}, entities_{std::move(entities)} {}

        template <typename C>
        auto with(const C& value = {}) -> batch_modifier& {
            world_->batch_add_component_<C>(entities_, value);
            return *this;
        }

        template <typename C>
        auto without() -> batch_modifier& {
            world_->batch_remove_component_<C>(entities_);
            return *this;
        }

        [[nodiscard]] auto get_entities() const -> const std::vector<entity>& {
            return entities_;
        }

        [[nodiscard]] auto release_entities() -> std::vector<entity> {
            return std::move(entities_);
        }

    private:
        world* world_;
        std::vector<entity> entities_;
    };

    world();
    ~world();

    world(const world&)                    = delete;
    auto operator=(const world&) -> world& = delete;
    world(world&&)                         = delete;
    auto operator=(world&&) -> world&      = delete;

    void update(float32 delta_time);
    void clear_changed();

    [[nodiscard]] auto create() -> modifier;
    [[nodiscard]] auto modify(entity ent) -> modifier;
    void destroy(entity ent) noexcept;

    [[nodiscard]] auto batch_create(uint32 count) -> batch_modifier;
    [[nodiscard]] auto batch_modify(std::vector<entity> entities) -> batch_modifier;
    void batch_destroy(const std::vector<entity>& entities) noexcept;

    template <typename T>
    [[nodiscard]] auto has(entity ent) const -> bool {
        return registry_.has<T>(ent);
    }

    template <typename T>
    [[nodiscard]] auto get(entity ent) -> T& {
        return registry_.get<T>(ent);
    }

    template <typename T>
    [[nodiscard]] auto get(entity ent) const -> const T& {
        return registry_.get<T>(ent);
    }

    template <typename... Cs>
    [[nodiscard]] auto view() -> component_view<Cs...> {
        return registry_.view<Cs...>();
    }

    template <typename... Cs, typename Fn>
    void for_each(Fn&& fn) {
        registry_.for_each<Cs...>(std::forward<Fn>(fn));
    }

    template <typename S>
    [[nodiscard]] auto system() -> S& {
        return std::get<S>(systems_);
    }

    template <typename S>
    [[nodiscard]] auto system() const -> const S& {
        return std::get<S>(systems_);
    }

    template <typename R>
    [[nodiscard]] auto resource() -> R& {
        return std::get<R>(resources_);
    }

    template <typename R>
    [[nodiscard]] auto resource() const -> const R& {
        return std::get<R>(resources_);
    }

    [[nodiscard]] auto registry() -> ecs::registry&;

    template <typename T>
    [[nodiscard]] auto changed() -> std::unordered_set<entity>& {
        return registry_.changed<T>();
    }

    [[nodiscard]] auto destroyed() const -> const std::vector<entity>&;

private:
    template <typename T>
    void add_component_(entity ent, T&& value = {}) {
        using C = std::remove_cvref_t<T>;
        remember_remove_hook_<C>();
        registry_.add<C>(ent, std::forward<T>(value));

        std::apply([&](auto&... systems) { (detail::invoke_on_add<C>(systems, ent), ...); },
                   systems_);
    }

    template <typename T>
    void remove_component_(entity ent) noexcept {
        std::apply([&](auto&... systems) { (detail::invoke_on_remove<T>(systems, ent), ...); },
                   systems_);
        registry_.remove<T>(ent);
    }

    template <typename T>
    void batch_add_component_(const std::vector<entity>& entities, const T& value = {}) {
        remember_remove_hook_<T>();
        registry_.batch_add<T>(entities, value);

        std::apply(
            [&](auto&... systems) {
                for (auto ent : entities) {
                    (detail::invoke_on_add<T>(systems, ent), ...);
                }
            },
            systems_);
    }

    template <typename T>
    void batch_remove_component_(const std::vector<entity>& entities) noexcept {
        std::apply(
            [&](auto&... systems) {
                for (auto ent : entities) {
                    (detail::invoke_on_remove<T>(systems, ent), ...);
                }
            },
            systems_);
        registry_.batch_remove<T>(entities);
    }

    // Destroying an entity has to notify the systems about every component it
    // still holds, and by then the type is only known through its id — so each
    // component type leaves behind a typed remover the first time it is added.
    template <typename T>
    void remember_remove_hook_() {
        const uint32 id = component_id_of<T>();
        if (id >= remove_hooks_.size()) {
            remove_hooks_.resize(id + 1, nullptr);
        }
        if (remove_hooks_[id] == nullptr) {
            remove_hooks_[id] = +[](world& w, entity ent) { w.remove_component_<T>(ent); };
        }
    }

    void detach_components_(entity ent) noexcept;

    ecs::registry registry_;
    systems systems_;
    resources resources_;
    std::vector<void (*)(world&, entity)> remove_hooks_;
};

class vox_serializer final {
public:
    using entity_names_type = std::unordered_map<entity, std::string>;
    using error_type        = vox_writer::error_type;

    struct options {
        std::optional<entity_names_type> entity_names;
        std::unordered_set<entity> excluded;
    };

    vox_serializer(world& world, vox_writer& writer, entity root, options opts = {});

    auto serialize(const std::filesystem::path& filepath) -> std::expected<void, error_type>;

    [[nodiscard]] auto extract() const -> asset::vox_prefab_data;

private:
    void generate_entity_names_();
    [[nodiscard]] auto extract_entity_(entity ent) const -> asset::vox_entity_data;

    world* world_;
    vox_writer* writer_;
    entity root_;
    entity_names_type entity_names_;
    std::unordered_set<entity> excluded_;
};

class vox_deserializer final {
public:
    using error_type = asset::vox_parser::error_type;

    struct options {
        bool skip_sockets = false;
        bool skip_targets = false;
    };

    struct result {
        std::string root_name;
        std::unordered_map<std::string, entity> name_to_entity;
        std::unordered_map<entity, std::string> entity_to_name;
        std::vector<entity> entities;
    };

    vox_deserializer(world& world, asset::vox_parser& parser);

    auto deserialize(const std::filesystem::path& filepath) -> std::expected<result, error_type>;
    auto deserialize(const std::filesystem::path& filepath, const options& opts)
        -> std::expected<result, error_type>;

private:
    void apply_entity_(const asset::vox_entity_data& data, result& res, const options& opts);

    world* world_;
    asset::vox_parser* parser_;
};

}  // namespace vw::ecs
