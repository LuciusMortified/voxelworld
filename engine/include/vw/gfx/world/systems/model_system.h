#pragma once

#ifndef VW_GFX_MODEL_SYSTEM_H
#define VW_GFX_MODEL_SYSTEM_H

#include <vector>

#include "vw/gfx/world/registry.h"
#include "vw/gfx/world/components/model_component.h"
#include "vw/gfx/resource/mesh_pool.h"

namespace vw::gfx {

class model;

template <typename... Cs>
class model_system {
public:
    using registry_type = registry<Cs...>;

    explicit model_system(registry_type& registry, mesh_pool& mesh_pool);

    void update();
    void set_model(entity e, std::shared_ptr<vw::gfx::model> model_ptr);

    class model_modifier {
    public:
        explicit model_modifier(model_system& system, model_component* component, entity e);
        
        void set_voxel(int x, int y, int z, const voxel& v);
        void set_voxel(int x, int y, int z, color c);
        void fill(const voxel& v);
        void clear();
        
    private:
        model_system* system_;
        model_component* component_;
        entity entity_;
    };

    auto modify(entity e) -> model_modifier;
    void mark_dirty(entity e);

private:
    void process_dirty_entities();
    void update_completed_meshes();

    registry_type& registry_;
    mesh_pool& mesh_pool_;
    std::vector<entity> pending_entities_;
    std::vector<entity> dirty_entities_;
};

template <typename... Cs>
struct model_system_from_tuple;

template <typename... Cs>
struct model_system_from_tuple<std::tuple<Cs...>> {
    using type = model_system<Cs...>;
};

}  // namespace vw::gfx

#include "vw/gfx/world/systems/model_system.inl.h"

#endif  // VW_GFX_MODEL_SYSTEM_H
