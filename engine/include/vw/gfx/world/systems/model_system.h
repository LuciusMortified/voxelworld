#pragma once

#ifndef VW_GFX_MODEL_SYSTEM_H
#define VW_GFX_MODEL_SYSTEM_H

#include <chrono>
#include <set>
#include <vector>

#include "vw/gfx/world/components/model_component.h"
#include "vw/gfx/world/system_trait.h"
#include "vw/gfx/world/world_context.h"

namespace vw::gfx {

struct model_system_stats {
    float32 process_completed_ms  = 0.0f;
    float32 update_completed_ms   = 0.0f;
    float32 process_dirty_ms      = 0.0f;
    uint32 pending_entities_count = 0;
    uint32 pending_meshes_count   = 0;
};

class model;

template <typename WC>
class model_system {
public:
    using registry_type = entity_registry_from_tuple<WC>::type;
    using context_type = world_context<WC>;

    explicit model_system(context_type& context);

    class model_modifier {
    public:
        explicit model_modifier(model_system& system, model_component* component, entity e);

        [[nodiscard]] std::shared_ptr<model> get_model() const;

        void set_model(std::shared_ptr<model> model_ptr);
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

    [[nodiscard]] auto get_stats() const -> const model_system_stats&;

private:
    void process_dirty_entities();
    void update_completed_meshes();

    context_type* context_;
    std::unordered_set<entity> pending_entities_;
    model_system_stats stats_;
};

}  // namespace vw::gfx

namespace vw::gfx { class model_registry; }

template <>
struct vw::gfx::system_trait<vw::gfx::model_system> {
    using components = std::tuple<vw::gfx::model_component>;
    using depends_on = vw::gfx::system_list<>;
    using resources  = std::tuple<vw::gfx::model_registry>;
};

#include "vw/gfx/world/systems/model_system.inl.h"

#endif  // VW_GFX_MODEL_SYSTEM_H
