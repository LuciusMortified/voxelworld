#pragma once

namespace vw::gfx {

inline auto animation_player_component::layer_count() const -> size_t {
    return layers_.size();
}

inline auto animation_player_component::has_layer(size_t index) const -> bool {
    return index < layers_.size();
}

inline auto animation_player_component::get_layer(size_t index) const -> const animation_layer& {
    return layers_[index];
}

inline auto animation_player_component::is_any_playing() const -> bool {
    for (const auto& layer : layers_) {
        if (layer.is_active()) {
            return true;
        }
    }
    return false;
}

}  // namespace vw::gfx
