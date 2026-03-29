#pragma once

#ifndef VW_GFX_WORLD_CONTEXT_H
#define VW_GFX_WORLD_CONTEXT_H

#include <memory>

#include "vw/gfx/world/entity_registry.h"

namespace vw::gfx {

class mesh_pool;

template <typename WC>
class world_grid;

template <typename WC>
struct world_context {
    using registry_type = entity_registry_from_tuple<WC>::type;

    registry_type& registry;
    mesh_pool* mesh_pool = nullptr;
    std::shared_ptr<world_grid<WC>> world_grid;
};

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_CONTEXT_H
