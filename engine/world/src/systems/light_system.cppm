export module vw.world:systems.light;

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

class light_system final {
public:
    explicit light_system(world& w);

    void update(float32 dt);

    class light_modifier {
    public:
        auto set_color(const vec3f& color) -> light_modifier&;
        auto set_intensity(float32 intensity) -> light_modifier&;
        auto set_range(float32 range) -> light_modifier&;

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

}  // namespace vw::ecs
