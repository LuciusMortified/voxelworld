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
    [[nodiscard]] auto create(std::string_view name, vec3i size) -> std::shared_ptr<model>;
    [[nodiscard]] auto create_unnamed(int width, int height, int depth) -> std::shared_ptr<model>;
    [[nodiscard]] auto create_unnamed(vec3i size) -> std::shared_ptr<model>;
    [[nodiscard]] auto create_clone(std::string_view name) -> std::shared_ptr<model>;
    void erase(std::string_view name);

    [[nodiscard]] auto get_identity_pool() -> model_identity_pool& { return identity_pool_; }
    [[nodiscard]] auto get_page_pool() -> page_pool& { return page_pool_; }

private:
    model_identity_pool identity_pool_;
    page_pool page_pool_;
    std::unordered_map<std::string, std::shared_ptr<model>> models_;
};

}  // namespace vw::gfx

#include "vw/gfx/model/model_registry.inl.h"

#endif  // VW_GFX_MODEL_REGISTRY_H
