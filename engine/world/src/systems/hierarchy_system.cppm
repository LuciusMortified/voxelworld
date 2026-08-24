export module vw.world:systems.hierarchy;

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

}  // namespace vw::ecs
