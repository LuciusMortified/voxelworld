#pragma once

#ifndef VW_GFX_HIERARCHY_COMPONENT_INL_H
#define VW_GFX_HIERARCHY_COMPONENT_INL_H

#include <ranges>
#include <algorithm>

#include "vw/gfx/world/components/hierarchy_component.h"

namespace vw::gfx {

inline auto hierarchy_component::has_parent() const -> bool {
    return parent_.is_valid();
}

inline auto hierarchy_component::get_parent() const -> entity {
    return parent_;
}

inline auto hierarchy_component::has_child(entity child) const -> bool {
    return std::ranges::contains(children_, child);
}

inline auto hierarchy_component::get_children() const -> const std::vector<entity>& {
    return children_;
}

}  // namespace vw::gfx

#endif  // VW_GFX_HIERARCHY_COMPONENT_INL_H
