#pragma once

#ifndef VW_GFX_MODEL_REGISTRY_H
#define VW_GFX_MODEL_REGISTRY_H

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "vw/gfx/model/model.h"
#include "vw/gfx/model/model_identity_pool.h"

namespace vw::gfx {

class model_registry {
public:
    [[nodiscard]] auto has(std::string_view name) const -> bool;
    [[nodiscard]] auto get(std::string_view name) const -> std::shared_ptr<model>;
    [[nodiscard]] auto create(std::string_view name, int width, int height, int depth)
        -> std::shared_ptr<model>;
    [[nodiscard]] auto create_unnamed(int width, int height, int depth) -> std::shared_ptr<model>;
    [[nodiscard]] auto create_clone(std::string_view name) -> std::shared_ptr<model>;

private:
    model_identity_pool identity_pool_;
    std::unordered_map<std::string, std::shared_ptr<model>> models_;
};

}  // namespace vw::gfx

#include "vw/gfx/model/model_registry.inl.h"

#endif  // VW_GFX_MODEL_REGISTRY_H
