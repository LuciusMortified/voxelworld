export module vw.world:systems.animation_fsm;

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

}  // namespace vw::ecs
