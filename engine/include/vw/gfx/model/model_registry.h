#pragma once

#ifndef VW_GFX_MODEL_REGISTRY_H
#define VW_GFX_MODEL_REGISTRY_H

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "vw/gfx/model/model.h"

namespace vw::gfx {

class model_registry {
public:
    void add(std::string_view name, std::shared_ptr<model> model);
    [[nodiscard]] auto has(std::string_view name) const -> bool;
    [[nodiscard]] auto get(std::string_view name) const -> std::shared_ptr<model>;
    [[nodiscard]] auto clone(std::string_view name) const -> std::shared_ptr<model>;
    void remove(std::string_view name);

private:
    std::unordered_map<std::string, std::shared_ptr<model>> models_;
};

}  // namespace vw::gfx

#include "vw/gfx/model/model_registry.inl.h"

#endif  // VW_GFX_MODEL_REGISTRY_H
