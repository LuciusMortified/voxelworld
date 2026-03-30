#pragma once

#ifndef VW_GFX_HIERARCHY_COMPONENT_H
#define VW_GFX_HIERARCHY_COMPONENT_H

#include <vector>

#include "vw/gfx/world/entity.h"

namespace vw::gfx {

struct hierarchy_component final {
private:
    entity parent_;
    std::vector<entity> children_;

public:
    [[nodiscard]] auto has_parent() const -> bool;
    [[nodiscard]] auto get_parent() const -> entity;

    [[nodiscard]] auto has_child(entity child) const -> bool;
    [[nodiscard]] auto get_children() const -> const std::vector<entity>&;

    template <typename>
    friend class hierarchy_system;

    template <typename>
    friend class transform_system;
};

}  // namespace vw::gfx

#include "vw/gfx/world/components/hierarchy_component.inl.h"

#endif  // VW_GFX_HIERARCHY_COMPONENT_H