#pragma once

namespace vw::gfx {

template <typename T>
auto keyframe<T>::operator<(const keyframe& other) const -> bool {
    return time < other.time;
}

template <typename T>
auto keyframe<T>::operator==(const keyframe& other) const -> bool {
    return time == other.time;
}

}  // namespace vw::gfx
