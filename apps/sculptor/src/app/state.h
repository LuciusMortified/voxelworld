#pragma once

#ifndef VW_SCULPTOR_STATE_H
#define VW_SCULPTOR_STATE_H

namespace vw::sculptor {

struct entity_state {
    std::string name;
    gfx::entity parent = gfx::invalid_entity;
    std::vector<gfx::entity> children;
    size_t depth = 0;
    vec3i model_size;
};

struct state {
    color selected_color = colors::white;

    gfx::entity selected_entity = gfx::invalid_entity;
    gfx::entity root_entity     = gfx::invalid_entity;
    std::unordered_map<gfx::entity, entity_state> entity_map;
};

}  // namespace vw::sculptor

#include "state.inl.h"

#endif  // VW_SCULPTOR_STATE_H
