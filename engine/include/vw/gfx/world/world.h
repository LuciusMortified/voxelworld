#pragma once

#ifndef VW_GFX_WORLD_H
#define VW_GFX_WORLD_H

#include <optional>
#include <unordered_set>

#include "vw/gfx/animation/animation_clip_registry.h"
#include "vw/gfx/model/model_registry.h"
#include "vw/gfx/resource/mesh_pool.h"
#include "vw/gfx/spatial/ray.h"
#include "vw/gfx/world/entity_registry.h"
#include "vw/gfx/world/systems/animation_fsm_system.h"
#include "vw/gfx/world/systems/animation_system.h"
#include "vw/gfx/world/systems/character_controller_system.h"
#include "vw/gfx/world/systems/hierarchy_system.h"
#include "vw/gfx/world/systems/light_system.h"
#include "vw/gfx/world/systems/model_system.h"
#include "vw/gfx/world/systems/physics_system.h"
#include "vw/gfx/world/systems/socket_system.h"
#include "vw/gfx/world/systems/spatial_system.h"
#include "vw/gfx/world/systems/transform_system.h"
#include "vw/gfx/world/systems/world_grid_system.h"
#include "vw/gfx/world/world_components.h"
#include "vw/gfx/world/world_context.h"

namespace vw::gfx {

struct world_update_stats {
    float32 character_controller_ms = 0.0f;
    float32 physics_ms              = 0.0f;
    float32 transform_ms            = 0.0f;
    float32 model_ms                = 0.0f;
    float32 spatial_ms              = 0.0f;
    float32 light_ms                = 0.0f;
    float32 world_grid_ms           = 0.0f;
    float32 animation_fsm_ms = 0.0f;
    float32 animation_ms               = 0.0f;
};

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
    using registry_type                    = entity_registry_from_tuple<WC>::type;
    using context_type                     = world_context<WC>;
    using transform_system_type            = transform_system<WC>;
    using hierarchy_system_type            = hierarchy_system<WC>;
    using model_system_type                = model_system<WC>;
    using spatial_system_type              = spatial_system<WC>;
    using light_system_type                = light_system<WC>;
    using socket_system_type               = socket_system<WC>;
    using animation_system_type                = animation_system<WC>;
    using animation_fsm_system_type = animation_fsm_system<WC>;
    using character_controller_system_type     = character_controller_system<WC>;
    using physics_system_type              = physics_system<WC>;
    using world_grid_system_type           = world_grid_system<WC>;

    explicit world(vulkan_context& context, const block_registry& registry);
    ~world();

    world(const world&)                    = delete;
    auto operator=(const world&) -> world& = delete;
    world(world&&)                         = delete;
    auto operator=(world&&) -> world&      = delete;

    void update(float32 delta_time);

    template <typename T>
    [[nodiscard]] auto has_component(entity ent) const -> bool;

    template <typename T>
    [[nodiscard]] auto get_component(entity ent) -> T&;

    template <typename T>
    [[nodiscard]] auto get_component(entity ent) const -> const T&;

    [[nodiscard]] auto get_mesh_pool() const -> const mesh_pool&;
    [[nodiscard]] auto get_mesh_pool() -> mesh_pool&;

    template <typename... Cs>
    [[nodiscard]] auto view_components() -> component_view<registry_type, Cs...>;

    [[nodiscard]] auto get_hierarchy_system() -> hierarchy_system_type&;

    [[nodiscard]] auto get_transform_system() -> transform_system_type&;

    [[nodiscard]] auto get_model_registry() -> model_registry&;

    [[nodiscard]] auto get_model_system() -> model_system_type&;

    [[nodiscard]] auto get_spatial_system() -> spatial_system_type&;

    [[nodiscard]] auto get_light_system() -> light_system_type&;

    [[nodiscard]] auto get_socket_system() -> socket_system_type&;

    [[nodiscard]] auto get_animation_system() -> animation_system_type&;

    [[nodiscard]] auto get_animation_fsm_system() -> animation_fsm_system_type&;

    [[nodiscard]] auto get_animation_fsm_system() const -> const animation_fsm_system_type&;

    [[nodiscard]] auto get_animation_clip_registry() -> animation_clip_registry&;

    void set_world_grid(std::shared_ptr<world_grid<WC>> grid);
    [[nodiscard]] auto get_world_grid() const -> std::shared_ptr<world_grid<WC>>;

    [[nodiscard]] auto get_world_grid_system() -> world_grid_system_type&;

    [[nodiscard]] auto get_character_controller_system() -> character_controller_system_type&;

    [[nodiscard]] auto get_physics_system() -> physics_system_type&;

    [[nodiscard]] auto get_registry() -> registry_type&;

    template <typename T>
    [[nodiscard]] auto changed() -> std::unordered_set<entity>&;

    [[nodiscard]] auto voxel_ray_cast(const ray& r, std::unordered_set<entity>& candidates) const
        -> std::optional<voxel_ray_hit>;

    [[nodiscard]] auto destroyed() const -> const std::vector<entity>&;

    [[nodiscard]] auto get_update_stats() const -> const world_update_stats&;

    void clear_changed();

private:
    template <typename T>
    void add_component(entity ent, T&& value = {});

    template <typename T>
    void remove_component(entity ent) noexcept;

    [[nodiscard]] auto create_entity() -> entity;
    void destroy_entity(entity ent) noexcept;

    [[nodiscard]] auto batch_create_entities(uint32 count) -> std::vector<entity>;
    void batch_destroy_entities(const std::vector<entity>& entities) noexcept;

    registry_type registry_;
    mesh_pool mesh_pool_;
    context_type context_;
    spatial_system_type spatial_system_;
    transform_system_type transform_system_;
    hierarchy_system_type hierarchy_system_;
    model_registry model_registry_;
    model_system_type model_system_;
    light_system_type light_system_;
    socket_system_type socket_system_;
    animation_clip_registry animation_clip_registry_;
    animation_system_type animation_system_;
    animation_fsm_system_type animation_fsm_system_;
    character_controller_system_type character_controller_system_;
    physics_system_type physics_system_;
    world_grid_system_type world_grid_system_;
    world_update_stats update_stats_;
};

}  // namespace vw::gfx

#include "vw/gfx/world/world.inl.h"

#endif  // VW_GFX_WORLD_H
