#pragma once

#ifndef VW_GFX_MODEL_COMPONENT_H
#define VW_GFX_MODEL_COMPONENT_H

#include <memory>

#include "vw/core.h"
#include "vw/core/color.h"
#include "vw/core/voxel.h"
#include "vw/gfx/model/model.h"

namespace vw::gfx {

template <typename... Cs>
class model_system;

struct model_component final {
private:
    std::shared_ptr<model> model_;

    template <typename... Cs>
    friend class model_system;

public:
    [[nodiscard]] auto get_voxel(
        int x, int y, int z
    ) const -> voxel {
        return model_->get_voxel(x, y, z);
    }

    [[nodiscard]] auto get_voxel(vec3i pos) const -> voxel {
        return model_->get_voxel(pos);
    }

    [[nodiscard]] auto get_voxels() const -> const std::vector<voxel>& {
        return model_->get_voxels();
    }

    [[nodiscard]] auto is_empty(
        int x, int y, int z
    ) const -> bool {
        return model_->is_empty(x, y, z);
    }

    [[nodiscard]] auto width() const -> int {
        return model_->width();
    }

    [[nodiscard]] auto height() const -> int {
        return model_->height();
    }

    [[nodiscard]] auto depth() const -> int {
        return model_->depth();
    }

    [[nodiscard]] auto size() const -> vec3i {
        return model_->size();
    }

    [[nodiscard]] auto has_model() const -> bool {
        return model_ != nullptr;
    }

    [[nodiscard]] auto get_identity() const -> model_identity {
        return model_ ? model_->get_identity() : invalid_model_identity;
    }
};

}  // namespace vw::gfx

#endif  // VW_GFX_MODEL_COMPONENT_H
