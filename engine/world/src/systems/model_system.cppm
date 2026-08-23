export module vw.world:systems.model;

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

class model_system {
public:
    explicit model_system(world& w);

    class model_modifier {
    public:
        explicit model_modifier(model_system& system, model_component* component, entity e);

        [[nodiscard]] auto get_model() const -> std::shared_ptr<asset::model>;

        void set_model(std::shared_ptr<asset::model> model_ptr);
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

}  // namespace vw::ecs
