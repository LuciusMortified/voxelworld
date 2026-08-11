module vw.core;

import std;

namespace vw::spatial {

namespace {

auto normalized_plane(float32 x, float32 y, float32 z, float32 w) -> plane {
    const vec3f normal{x, y, z};
    const float32 len = math::length(normal);
    if (len <= 0.0F) {
        return {};
    }
    const float32 inv_len = 1.0F / len;
    return {.normal = normal * inv_len, .distance = w * inv_len};
}

}  // namespace

auto frustum::from_view_projection_matrix(const mat4f& view_proj) -> frustum {
    frustum f{};

    for (std::size_t row = 0; row < 3; ++row) {
        const std::size_t left  = row * 2;
        const std::size_t right = left + 1;

        f.planes[left] = normalized_plane(
            view_proj[3, 0] + view_proj[row, 0], view_proj[3, 1] + view_proj[row, 1],
            view_proj[3, 2] + view_proj[row, 2], view_proj[3, 3] + view_proj[row, 3]);

        f.planes[right] = normalized_plane(
            view_proj[3, 0] - view_proj[row, 0], view_proj[3, 1] - view_proj[row, 1],
            view_proj[3, 2] - view_proj[row, 2], view_proj[3, 3] - view_proj[row, 3]);
    }

    return f;
}

auto frustum::operator==(const frustum& other) const -> bool {
    for (std::size_t i = 0; i < 6; ++i) {
        if (planes[i].normal != other.planes[i].normal ||
            std::abs(planes[i].distance - other.planes[i].distance) > 1e-5F) {
            return false;
        }
    }
    return true;
}

auto frustum::operator!=(const frustum& other) const -> bool {
    return !(*this == other);
}

auto frustum::approximately_equal(
    const frustum& other, float32 angle_threshold, float32 distance_threshold) const -> bool {
    for (std::size_t i = 0; i < 6; ++i) {
        const float32 angle_diff = std::acos(
            math::clamp(math::dot(planes[i].normal, other.planes[i].normal), -1.0F, 1.0F));
        const float32 distance_diff = std::abs(planes[i].distance - other.planes[i].distance);

        if (angle_diff > angle_threshold || distance_diff > distance_threshold) {
            return false;
        }
    }
    return true;
}

}  // namespace vw::spatial
