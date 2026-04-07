#pragma once

namespace vw::gfx {

inline auto animation_target_component::get_name() const -> const std::string& {
    return target_name_;
}

inline auto animation_target_component::get_rest_transform() const -> const transform& {
    return rest_transform_;
}

}  // namespace vw::gfx
