#pragma once

#ifndef VW_GFX_MODEL_COMPONENT_H
#define VW_GFX_MODEL_COMPONENT_H

#include <memory>

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
        return model_ ? model_->get_voxel(x, y, z) : voxel{};
    }

    [[nodiscard]] auto is_empty(
        int x, int y, int z
    ) const -> bool {
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

    [[nodiscard]] auto has_model() const -> bool {
        return model_ != nullptr;
    }

    [[nodiscard]] auto get_identity() const -> model_identity {
        return model_ ? model_->get_identity() : model_identity{};
    }
};

}  // namespace vw::gfx

#endif  // VW_GFX_MODEL_COMPONENT_H
