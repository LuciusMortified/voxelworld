#pragma once

#ifndef VW_GFX_MODEL_COMPONENT_H
#define VW_GFX_MODEL_COMPONENT_H

#include <memory>

#include "vw/core.h"
#include "vw/core/voxel.h"
#include "vw/gfx/model/model.h"

namespace vw::gfx {

template <typename>
class model_system;

struct model_component final {
private:
    std::shared_ptr<model> model_;

    template <typename>
    friend class model_system;

public:
    [[nodiscard]] auto has_model() const -> bool;
    [[nodiscard]] auto get_model() const -> std::shared_ptr<model>;

    [[nodiscard]] auto get_voxel(int x, int y, int z) const -> voxel;
    [[nodiscard]] auto get_voxel(vec3i pos) const -> voxel;

    [[nodiscard]] auto is_empty(int x, int y, int z) const -> bool;
    [[nodiscard]] auto is_empty(vec3i pos) const -> bool;

    [[nodiscard]] auto width() const -> int;
    [[nodiscard]] auto height() const -> int;
    [[nodiscard]] auto depth() const -> int;
    [[nodiscard]] auto size() const -> vec3i;

    [[nodiscard]] auto get_identity() const -> model_identity;
};

}  // namespace vw::gfx

#include "model_component.inl.h"

#endif  // VW_GFX_MODEL_COMPONENT_H
