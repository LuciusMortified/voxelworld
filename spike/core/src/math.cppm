export module vw.core.spike:math;

import std;
import :types;

export namespace vw {

template <typename T>
struct vec3 {
    T x{};
    T y{};
    T z{};
};

using vec3f = vec3<float32>;
using vec3i = vec3<int32>;

[[nodiscard]] auto length(vec3f v) -> float32;
[[nodiscard]] auto describe(vec3f v) -> std::string;

}  // namespace vw
