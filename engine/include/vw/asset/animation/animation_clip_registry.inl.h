#pragma once

namespace vw::asset {

inline auto animation_clip_registry::create(std::string_view name)
    -> std::shared_ptr<animation_clip> {
    std::string name_str(name);
    auto clip = std::make_shared<animation_clip>(name_str);
    clips_[std::move(name_str)] = clip;
    return clip;
}

inline void animation_clip_registry::add(
    std::string_view name, std::shared_ptr<animation_clip> clip
) {
    std::string name_str(name);
    clips_[std::move(name_str)] = std::move(clip);
}

inline auto animation_clip_registry::get(std::string_view name) const
    -> std::shared_ptr<animation_clip> {
    auto it = clips_.find(name);
    if (it != clips_.end()) {
        return it->second;
    }
    return nullptr;
}

inline auto animation_clip_registry::has(std::string_view name) const -> bool {
    return clips_.find(name) != clips_.end();
}

inline void animation_clip_registry::remove(std::string_view name) {
    auto it = clips_.find(name);
    if (it != clips_.end()) {
        clips_.erase(it);
    }
}

inline auto animation_clip_registry::all() const -> const animation_clip_registry::map_type& {
    return clips_;
}

inline auto animation_clip_registry::count() const -> size_t {
    return clips_.size();
}

inline void animation_clip_registry::clear() {
    clips_.clear();
}

}  // namespace vw::asset
