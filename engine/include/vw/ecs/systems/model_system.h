#pragma once

#ifndef VW_ECS_MODEL_SYSTEM_H
#define VW_ECS_MODEL_SYSTEM_H

#include "vw/ecs/components/model_component.h"
#include "vw/ecs/entity_registry.h"
#include "vw/ecs/system_trait.h"

namespace vw::asset { class model; }

namespace vw::ecs {

class world;

class model_system {
public:
    using world_type    = world;
    using registry_type = registry;

    explicit model_system(world_type& w);

    class model_modifier {
    public:
        explicit model_modifier(model_system& system, model_component* component, entity e);

        [[nodiscard]] std::shared_ptr<vw::asset::model> get_model() const;

        void set_model(std::shared_ptr<vw::asset::model> model_ptr);
        void set_top_brightness(bool enabled);
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

}  // namespace vw::ecs

#endif  // VW_ECS_MODEL_SYSTEM_H
