#pragma once

namespace vw::gfx {

inline auto animation_clip_registry::create(std::string_view name)
    -> std::shared_ptr<animation_clip> {
    auto clip = std::make_shared<animation_clip>(std::string(name));
    clips_[std::string(name)] = clip;
    return clip;
}

inline auto animation_clip_registry::get(std::string_view name) const
    -> std::shared_ptr<animation_clip> {
    auto it = clips_.find(std::string(name));
    if (it != clips_.end()) {
        return it->second;
    }
    return nullptr;
}

inline auto animation_clip_registry::has(std::string_view name) const -> bool {
    return clips_.find(std::string(name)) != clips_.end();
}

inline void animation_clip_registry::remove(std::string_view name) {
    clips_.erase(std::string(name));
}

inline auto animation_clip_registry::all() const
    -> const std::unordered_map<std::string, std::shared_ptr<animation_clip>>& {
    return clips_;
}

inline auto animation_clip_registry::count() const -> size_t {
    return clips_.size();
}

inline void animation_clip_registry::clear() {
    clips_.clear();
}

}  // namespace vw::gfx
