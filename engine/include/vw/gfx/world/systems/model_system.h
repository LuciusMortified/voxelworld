#pragma once

#ifndef VW_GFX_MODEL_SYSTEM_H
#define VW_GFX_MODEL_SYSTEM_H

#include "vw/gfx/world/components/model_component.h"
#include "vw/gfx/world/entity_registry.h"
#include "vw/gfx/world/system_trait.h"

namespace vw::asset { class model; }

namespace vw::gfx {

template <typename>
class world;

template <typename WD>
class model_system {
public:
    using world_type    = world<WD>;
    using components    = typename WD::components;
    using registry_type = typename entity_registry_from_tuple<components>::type;

    explicit model_system(world_type& w);

    class model_modifier {
    public:
        explicit model_modifier(model_system& system, model_component* component, entity e);

        [[nodiscard]] std::shared_ptr<vw::asset::model> get_model() const;

        void set_model(std::shared_ptr<vw::asset::model> model_ptr);
        void set_voxel(int x, int y, int z, const voxel& v);
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
    world_type* world_;
};

}  // namespace vw::gfx

template <>
struct vw::gfx::system_trait<vw::gfx::model_system> {
    using components = std::tuple<vw::gfx::model_component>;
    using resources  = std::tuple<vw::asset::model_registry>;
};

#include "vw/gfx/world/systems/model_system.inl.h"

#endif  // VW_GFX_MODEL_SYSTEM_H
