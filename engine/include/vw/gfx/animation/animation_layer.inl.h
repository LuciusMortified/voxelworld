#pragma once

namespace vw::gfx {

inline auto animation_layer::is_active() const -> bool {
    return state == animation_state::playing || fade_is_out;
}

inline auto animation_layer::affects_target(const std::string& name) const -> bool {
    return mask.contains(name);
}

}  // namespace vw::gfx
