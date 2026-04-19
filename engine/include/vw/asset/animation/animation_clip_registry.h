#pragma once

#ifndef VW_ASSET_ANIMATION_CLIP_REGISTRY_H
#define VW_ASSET_ANIMATION_CLIP_REGISTRY_H

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "vw/asset/animation/animation_clip.h"

namespace vw::asset {

struct string_hash {
    using is_transparent = void;

    [[nodiscard]] auto operator()(
        std::string_view sv
    ) const noexcept -> size_t {
        return std::hash<std::string_view>{}(sv);
    }
};

class animation_clip_registry final {
public:
    using map_type = std::
        unordered_map<std::string, std::shared_ptr<animation_clip>, string_hash, std::equal_to<>>;

    [[nodiscard]] auto create(std::string_view name) -> std::shared_ptr<animation_clip>;
    void add(std::string_view name, std::shared_ptr<animation_clip> clip);
    [[nodiscard]] auto get(std::string_view name) const -> std::shared_ptr<animation_clip>;
    [[nodiscard]] auto has(std::string_view name) const -> bool;
    void remove(std::string_view name);
    [[nodiscard]] auto all() const -> const map_type&;
    [[nodiscard]] auto count() const -> size_t;
    void clear();

private:
    map_type clips_;
};

}  // namespace vw::asset

#include "vw/asset/animation/animation_clip_registry.inl.h"

#endif  // VW_ASSET_ANIMATION_CLIP_REGISTRY_H
