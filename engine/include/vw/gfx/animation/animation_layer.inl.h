#pragma once

namespace vw::gfx {

inline auto animation_layer::is_active() const -> bool {
    if (state == animation_state::playing || fade_is_out) {
        return true;
    }
    return blend_transition.duration > 0.0f && blend_elapsed < blend_transition.duration;
}

inline auto animation_layer::affects_target(const std::string& name) const -> bool {
    return mask.contains(name);
}

}  // namespace vw::gfx
