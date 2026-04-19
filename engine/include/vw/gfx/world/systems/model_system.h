#pragma once

#ifndef VW_GFX_MODEL_SYSTEM_H
#define VW_GFX_MODEL_SYSTEM_H

#include "vw/gfx/world/components/model_component.h"
#include "vw/gfx/world/system_trait.h"
#include "vw/gfx/world/world_context.h"

namespace vw::asset { class model; }

namespace vw::gfx {


template <typename WD>
class model_system {
public:
    using components = typename WD::components;
    using registry_type = typename entity_registry_from_tuple<components>::type;
    using context_type = world_context<WD>;

    explicit model_system(context_type& context);

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
    void on_add(entity e) {
        context_->registry().template request_change<model_component>(e);
    }

private:
    context_type* context_;
};

}  // namespace vw::gfx

template <>
struct vw::gfx::system_trait<vw::gfx::model_system> {
    using components = std::tuple<vw::gfx::model_component>;
    using resources  = std::tuple<vw::asset::model_registry>;
};

#include "vw/gfx/world/systems/model_system.inl.h"

#endif  // VW_GFX_MODEL_SYSTEM_H
