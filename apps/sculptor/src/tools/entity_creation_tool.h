#pragma once

#ifndef VW_SCULPTOR_ENTITY_CREATION_TOOL_H
#define VW_SCULPTOR_ENTITY_CREATION_TOOL_H

#include <vw/gfx/engine/engine.h>
#include <vw/gfx/world/world_components.h>

#include "app/state.h"

namespace vw::sculptor {

template <typename WC = gfx::base_world_components>
class entity_creation_tool {
public:
    using engine_type = gfx::engine<WC>;

    entity_creation_tool(engine_type& eng, state& st);

    struct creation_params {
        std::string_view name;
        gfx::entity parent = gfx::invalid_entity;
        vec3i model_size   = vec3i{6, 6, 6};
    };

    void execute(const creation_params& params);

private:
    engine_type* engine_;
    state* state_;
};

}  // namespace vw::sculptor

#include "entity_creation_tool.inl.h"

#endif  // VW_SCULPTOR_ENTITY_CREATION_TOOL_H
