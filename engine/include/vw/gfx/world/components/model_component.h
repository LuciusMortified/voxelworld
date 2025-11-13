#pragma once

#ifndef VW_GFX_MODEL_COMPONENT_H
#define VW_GFX_MODEL_COMPONENT_H

#include <memory>

#include "vw/core/color.h"
#include "vw/core/voxel.h"
#include "vw/gfx/model/model.h"
#include "vw/gfx/resource/mesh.h"

namespace vw::gfx {

template <typename... Cs>
class model_system;

struct model_component final {
private:
    std::shared_ptr<model> model_;
    std::shared_ptr<mesh> mesh_;
    bool dirty_ = true;

    template <typename... Cs>
    friend class model_system;

public:
    [[nodiscard]] auto get_voxel(int x, int y, int z) const -> voxel {
        return model_ ? model_->get_voxel(x, y, z) : voxel{};
    }

    [[nodiscard]] auto is_empty(int x, int y, int z) const -> bool {
        return model_ ? model_->is_empty(x, y, z) : true;
    }

    [[nodiscard]] auto width() const -> int {
        return model_ ? model_->width() : 0;
    }

    [[nodiscard]] auto height() const -> int {
        return model_ ? model_->height() : 0;
    }

    [[nodiscard]] auto depth() const -> int {
        return model_ ? model_->depth() : 0;
    }

    [[nodiscard]] auto get_mesh() const -> std::shared_ptr<mesh> {
        return mesh_;
    }
};

}  // namespace vw::gfx

#endif  // VW_GFX_MODEL_COMPONENT_H
