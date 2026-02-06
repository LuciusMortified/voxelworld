#pragma once

#include <algorithm>
#include <cmath>

namespace vw::gfx {

inline auto lerp(const vec3f& a, const vec3f& b, float32 t) -> vec3f {
    return vec3f{
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
    };
}

inline auto ease_in(const vec3f& a, const vec3f& b, float32 t) -> vec3f {
    float32 eased_t = t * t;  // Quadratic ease-in
    return lerp(a, b, eased_t);
}

inline auto ease_out(const vec3f& a, const vec3f& b, float32 t) -> vec3f {
    float32 eased_t = t * (2.0f - t);  // Quadratic ease-out
    return lerp(a, b, eased_t);
}

inline auto ease_in_out(const vec3f& a, const vec3f& b, float32 t) -> vec3f {
    float32 eased_t;
    if (t < 0.5f) {
        eased_t = 2.0f * t * t;  // Ease-in for first half
    } else {
        eased_t = -1.0f + (4.0f - 2.0f * t) * t;  // Ease-out for second half
    }
    return lerp(a, b, eased_t);
}

// Вычислить значение кубической кривой Безье для параметра t
inline auto cubic_bezier_scalar(float32 t, float32 p0, float32 p1, float32 p2, float32 p3)
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
    const vec2f& control1,
    const vec2f& control2
) -> vec3f {
    // Для упрощения используем Bezier только для времени (easing curve)
    // а затем применяем линейную интерполяцию к значениям
    float32 bezier_t = cubic_bezier_scalar(t, 0.0f, control1.y, control2.y, 1.0f);
    return lerp(a, b, bezier_t);
}

inline auto apply_easing(float32 t, interpolation_type type) -> float32 {
    // Clamp t to [0, 1]
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
            // Для cubic_bezier нужны контрольные точки - используем линейную как fallback
            return t;

        default:
            return t;
    }
}

inline auto apply_easing_bezier(
    float32 t,
    interpolation_type type,
    const vec2f& control1,
    const vec2f& control2
) -> float32 {
    if (type == interpolation_type::cubic_bezier) {
        // Clamp t to [0, 1]
        t = std::clamp(t, 0.0f, 1.0f);
        return cubic_bezier_scalar(t, 0.0f, control1.y, control2.y, 1.0f);
    } else {
        return apply_easing(t, type);
    }
}

inline auto interpolate(
    const vec3f& a,
    const vec3f& b,
    float32 t,
    interpolation_type type,
    const vec2f& control1,
    const vec2f& control2
) -> vec3f {
    switch (type) {
        case interpolation_type::linear:
            return lerp(a, b, t);

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
            return lerp(a, b, t);
    }
}

}  // namespace vw::gfx
