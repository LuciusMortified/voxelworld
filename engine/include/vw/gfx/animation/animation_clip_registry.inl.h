#pragma once

namespace vw::gfx {

inline void animation_clip_registry::register_clip(
    const std::string& name,
    std::shared_ptr<animation_clip> clip
) {
    if (!clip) {
        return;
    }

    clips_[name] = std::move(clip);
}

inline auto animation_clip_registry::get_clip(const std::string& name) const
    -> std::shared_ptr<animation_clip> {
    auto it = clips_.find(name);
    if (it != clips_.end()) {
        return it->second;
    }
    return nullptr;
}

inline auto animation_clip_registry::has_clip(const std::string& name) const -> bool {
    return clips_.find(name) != clips_.end();
}

inline void animation_clip_registry::unregister_clip(const std::string& name) {
    clips_.erase(name);
}

inline auto animation_clip_registry::create_clip(const std::string& name)
    -> std::shared_ptr<animation_clip> {
    auto clip = std::make_shared<animation_clip>(name);
    register_clip(name, clip);
    return clip;
}

inline auto animation_clip_registry::get_all_clips() const
    -> const std::unordered_map<std::string, std::shared_ptr<animation_clip>>& {
    return clips_;
}

inline auto animation_clip_registry::clip_count() const -> size_t {
    return clips_.size();
}

inline void animation_clip_registry::clear() {
    clips_.clear();
}

}  // namespace vw::gfx
