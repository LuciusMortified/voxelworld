#pragma once

#include <algorithm>
#include <cmath>

namespace vw {

inline auto ease_in(const vec3f& a, const vec3f& b, float32 t) -> vec3f {
    float32 eased_t = t * t;
    return math::lerp(a, b, eased_t);
}

inline auto ease_out(const vec3f& a, const vec3f& b, float32 t) -> vec3f {
    float32 eased_t = t * (2.0f - t);
    return math::lerp(a, b, eased_t);
}

inline auto ease_in_out(const vec3f& a, const vec3f& b, float32 t) -> vec3f {
    float32 eased_t;
    if (t < 0.5f) {
        eased_t = 2.0f * t * t;
    } else {
        eased_t = -1.0f + (4.0f - 2.0f * t) * t;
    }
    return math::lerp(a, b, eased_t);
}

inline auto cubic_bezier(float32 t, float32 p0, float32 p1, float32 p2, float32 p3)
    -> float32 {
    float32 one_minus_t = 1.0f - t;
    float32 one_minus_t_sq = one_minus_t * one_minus_t;
    float32 one_minus_t_cb = one_minus_t_sq * one_minus_t;
    float32 t_sq = t * t;
    float32 t_cb = t_sq * t;

    return one_minus_t_cb * p0 + 3.0f * one_minus_t_sq * t * p1 +
           3.0f * one_minus_t * t_sq * p2 + t_cb * p3;
}

inline auto cubic_bezier(
    const vec3f& a,
    const vec3f& b,
    float32 t,
    float32 control1,
    float32 control2
) -> vec3f {
    float32 bezier_t = cubic_bezier(t, 0.0f, control1, control2, 1.0f);
    return math::lerp(a, b, bezier_t);
}

inline auto apply_easing(float32 t, interpolation_type type) -> float32 {
    t = std::clamp(t, 0.0f, 1.0f);

    switch (type) {
        case interpolation_type::linear:
            return t;

        case interpolation_type::step:
            return t < 1.0f ? 0.0f : 1.0f;

        case interpolation_type::ease_in:
            return t * t;

        case interpolation_type::ease_out:
            return t * (2.0f - t);

        case interpolation_type::ease_in_out:
            if (t < 0.5f) {
                return 2.0f * t * t;
            } else {
                return -1.0f + (4.0f - 2.0f * t) * t;
            }

        case interpolation_type::cubic_bezier:
            return t;

        default:
            return t;
    }
}

inline auto apply_easing_bezier(
    float32 t,
    interpolation_type type,
    float32 control1,
    float32 control2
) -> float32 {
    if (type == interpolation_type::cubic_bezier) {
        t = std::clamp(t, 0.0f, 1.0f);
        return cubic_bezier(t, 0.0f, control1, control2, 1.0f);
    } else {
        return apply_easing(t, type);
    }
}

inline auto interpolate(
    const vec3f& a,
    const vec3f& b,
    float32 t,
    interpolation_type type,
    float32 control1,
    float32 control2
) -> vec3f {
    switch (type) {
        case interpolation_type::linear:
            return math::lerp(a, b, t);

        case interpolation_type::step:
            return t < 1.0f ? a : b;

        case interpolation_type::ease_in:
            return ease_in(a, b, t);

        case interpolation_type::ease_out:
            return ease_out(a, b, t);

        case interpolation_type::ease_in_out:
            return ease_in_out(a, b, t);

        case interpolation_type::cubic_bezier:
            return cubic_bezier(a, b, t, control1, control2);

        default:
            return math::lerp(a, b, t);
    }
}

}  // namespace vw
