module vw.core.spike;

import std;

namespace vw {

auto length(vec3f v) -> float32 {
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

auto describe(vec3f v) -> std::string {
    return std::format("({}, {}, {})", v.x, v.y, v.z);
}

}  // namespace vw
