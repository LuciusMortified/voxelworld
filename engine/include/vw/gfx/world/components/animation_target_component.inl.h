#pragma once

namespace vw::gfx {

inline auto animation_target_component::get_name() const -> const std::string& {
    return target_name_;
}

}  // namespace vw::gfx
