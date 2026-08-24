export module vw.world:systems.character_controller;

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

}  // namespace vw::ecs
