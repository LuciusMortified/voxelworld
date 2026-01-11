#pragma once

#ifndef VW_GFX_MODEL_SYSTEM_H
#define VW_GFX_MODEL_SYSTEM_H

#include <set>
#include <vector>

#include "vw/gfx/world/registry.h"
#include "vw/gfx/world/components/model_component.h"
#include "vw/gfx/resource/mesh_pool.h"

namespace vw::gfx {

class model;

template <typename... Cs>
class spatial_system;

template <typename... Cs>
class model_system {
public:
    using registry_type = registry<Cs...>;
    using spatial_system_type = spatial_system<Cs...>;

    explicit model_system(
        registry_type& registry,
        mesh_pool& mesh_pool,
        spatial_system_type& spatial_sys
    );

    void update();

    class model_modifier {
    public:
        explicit model_modifier(model_system& system, model_component* component, entity e);

        void set_model(std::shared_ptr<model> model_ptr);
        void set_voxel(int x, int y, int z, const voxel& v);
        void set_voxel(int x, int y, int z, color c);
        void set_voxel(vec3i pos, const voxel& v);
        void set_voxel(vec3i pos, color c);
        void fill(const voxel& v);
        void clear();
        
    private:
        model_system* system_;
        model_component* component_;
        entity entity_;
    };

    auto modify(entity e) -> model_modifier;
    void mark_dirty(entity ent);

    [[nodiscard]]
    auto get_render_dirty_entities() -> std::unordered_set<entity>&;

    void mark_render_dirty(entity ent);

private:
    void process_dirty_entities();
    void update_completed_meshes();

    registry_type* registry_;
    mesh_pool* mesh_pool_;
    spatial_system_type* spatial_system_;
    std::unordered_set<entity> pending_entities_;
    std::unordered_set<entity> dirty_entities_;
    std::unordered_set<entity> render_dirty_entities_;
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
