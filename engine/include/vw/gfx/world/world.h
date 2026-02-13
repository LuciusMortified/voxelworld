#pragma once

#ifndef VW_GFX_WORLD_H
#define VW_GFX_WORLD_H

#include <optional>
#include <unordered_set>

#include "vw/gfx/animation/animation_clip_registry.h"
#include "vw/gfx/model/model_registry.h"
#include "vw/gfx/spatial/ray.h"
#include "vw/gfx/world/registry.h"
#include "vw/gfx/world/systems/animation_system.h"
#include "vw/gfx/world/systems/hierarchy_system.h"
#include "vw/gfx/world/systems/light_system.h"
#include "vw/gfx/world/systems/model_system.h"
#include "vw/gfx/world/systems/spatial_system.h"
#include "vw/gfx/world/systems/transform_system.h"
#include "vw/gfx/world/world_components.h"

namespace vw::gfx {

template <typename WC>
class entity_guard;

template <typename WC>
class entity_guard_group;

class vulkan_context;

template <typename WC = base_world_components>
class world final {
    friend class entity_guard<WC>;
    friend class entity_guard_group<WC>;

public:
    using registry_type         = registry_from_tuple<WC>::type;
    using transform_system_type = transform_system_from_tuple<WC>::type;
    using hierarchy_system_type = hierarchy_system_from_tuple<WC>::type;
    using model_system_type     = model_system_from_tuple<WC>::type;
    using spatial_system_type   = spatial_system_from_tuple<WC>::type;
    using light_system_type     = light_system_from_tuple<WC>::type;
    using animation_system_type = animation_system_from_tuple<WC>::type;

    explicit world(vulkan_context& context);
    ~world()                               = default;
    world(const world&)                    = delete;
    auto operator=(const world&) -> world& = delete;
    world(world&&)                         = delete;
    auto operator=(world&&) -> world&      = delete;

    void update(float32 delta_time);

    template <typename T>
    void add_component(entity ent, T&& value = {});

    template <typename T>
    [[nodiscard]] auto has_component(entity ent) const -> bool;

    template <typename T>
    [[nodiscard]] auto get_component(entity ent) -> T&;

    template <typename T>
    void remove_component(entity ent) noexcept;

    [[nodiscard]] auto get_mesh_pool() const -> const mesh_pool&;

    template <typename... Cs>
    [[nodiscard]] auto view_components() -> component_view<registry_type, Cs...>;

    [[nodiscard]] auto get_hierarchy_system() -> hierarchy_system_type&;

    [[nodiscard]] auto get_transform_system() -> transform_system_type&;

    [[nodiscard]] auto get_model_registry() -> model_registry&;

    [[nodiscard]] auto get_model_system() -> model_system_type&;

    [[nodiscard]] auto get_spatial_system() -> spatial_system_type&;

    [[nodiscard]] auto get_light_system() -> light_system_type&;

    [[nodiscard]] auto get_animation_system() -> animation_system_type&;

    [[nodiscard]] auto get_animation_clip_registry() -> animation_clip_registry&;

    [[nodiscard]] auto voxel_ray_cast(
        const ray& r,
        std::unordered_set<entity>& candidates
    ) const -> std::optional<voxel_ray_hit>;

private:
    [[nodiscard]] auto create_entity() -> entity;
    void destroy_entity(entity ent) noexcept;

    [[nodiscard]] auto batch_create_entities(uint32 count) -> std::vector<entity>;
    void batch_destroy_entities(const std::vector<entity>& entities) noexcept;

    registry_type registry_;
    mesh_pool mesh_pool_;
    spatial_system_type spatial_system_;
    transform_system_type transform_system_;
    hierarchy_system_type hierarchy_system_;
    model_registry model_registry_;
    model_system_type model_system_;
    light_system_type light_system_;
    animation_clip_registry animation_clip_registry_;
    animation_system_type animation_system_;
};

}  // namespace vw::gfx

#include "vw/gfx/world/world.inl.h"

#endif  // VW_GFX_WORLD_H
